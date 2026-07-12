#include "xmppstorage.h"

#include "notedata.h"
#include "securekeystore.h"
#include "utils.h"
#include "xmppsettingswidget.h"
#include "xmppworker.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QUuid>

#include <algorithm>
#include <utility>

namespace QtNote {

namespace {
    QString storageKeyName(const QString &jid)
    {
        return QStringLiteral("xmpp-storage-master-key-v1:%1").arg(jid.trimmed().section(QLatin1Char('/'), 0, 0));
    }

    template <typename Result, typename Function> Result invokeWorker(XmppWorker *worker, Function &&function)
    {
        Result result;

        if (QThread::currentThread() == worker->thread()) {
            return function();
        }

        const bool invoked
            = QMetaObject::invokeMethod(worker, [&]() { result = function(); }, Qt::BlockingQueuedConnection);

        if (!invoked) {
            result.ok    = false;
            result.error = QStringLiteral("Failed to invoke the XMPP worker");
        }
        return result;
    }

    QIcon themedIcon(const QString &primary, const QString &fallback)
    {
        auto icon = QIcon::fromTheme(primary);
        if (icon.isNull()) {
            icon = QIcon::fromTheme(fallback);
        }
        return icon;
    }

} // namespace

XmppStorage::XmppStorage(QObject *parent) : NoteStorage(parent)
{
    workerThread_.setObjectName(QStringLiteral("QtNoteXmppPubSubWorker"));
    worker_ = new XmppWorker;
    worker_->moveToThread(&workerThread_);

    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(worker_, &XmppWorker::remoteNotePublished, this, &XmppStorage::onRemoteNotePublished);
    connect(worker_, &XmppWorker::remoteNoteRetracted, this, &XmppStorage::onRemoteNoteRetracted);
    connect(worker_, &XmppWorker::remoteNodeInvalidated, this, &XmppStorage::onRemoteNodeInvalidated);
    connect(worker_, &XmppWorker::connectionChanged, this, &XmppStorage::onConnectionChanged);
    connect(worker_, &XmppWorker::workerError, this, [this](const QString &error) { reportError(error); });
    connect(worker_, &XmppWorker::storageKeyReceived, this, [this](const QByteArray &key) {
        const auto jid = readConfig().jid;
        if (jid.isEmpty())
            return;
        auto existing = SecureKeyStore::read(storageKeyName(jid));
        if (existing && existing.value != key) {
            const auto  existingId = SecureEnvelope::keyId(existing.value);
            const auto  receivedId = SecureEnvelope::keyId(key);
            QMessageBox dialog(QMessageBox::Warning, tr("Different XMPP storage key received"),
                               tr("A trusted own device sent a storage key that differs from the key configured on "
                                  "this device."),
                               QMessageBox::NoButton, QApplication::activeWindow());
            dialog.setInformativeText(
                tr("Current key: %1\nReceived key: %2\n\nReplacing the key makes notes encrypted with the "
                   "current key unavailable unless it is restored later.")
                    .arg(QString::fromLatin1(existingId.left(8).toHex()),
                         QString::fromLatin1(receivedId.left(8).toHex())));
            dialog.setDetailedText(
                tr("Recovery key for the current key:\n%1").arg(SecureEnvelope::encodeRecoveryKey(existing.value)));
            auto *keep    = dialog.addButton(tr("Keep current key"), QMessageBox::RejectRole);
            auto *replace = dialog.addButton(tr("Replace with received key"), QMessageBox::DestructiveRole);
            dialog.setDefaultButton(keep);
            dialog.exec();
            if (dialog.clickedButton() != replace) {
                const auto message = tr("The current XMPP storage key was kept");
                emit       encryptionKeyChanged(existingId, message);
                return;
            }
        }
        installReceivedStorageKey(jid, key);
    });

    workerThread_.start();
}

void XmppStorage::installReceivedStorageKey(const QString &jid, const QByteArray &key)
{
    if (auto error = SecureKeyStore::write(storageKeyName(jid), key)) {
        emit encryptionKeyChanged({}, error.message);
        reportError(error.message);
        return;
    }
    config_ = readConfig();
    clearErrorState();
    emit       encryptionKeyChanged(SecureEnvelope::keyId(key), tr("Storage key received from a trusted device"));
    auto      *job                  = initAsync(this);
    const auto finishInitialization = [this, job]() {
        if (job->state() == StorageJob::Succeeded)
            emit invalidated();
        job->deleteLater();
    };
    connect(job, &StorageJob::finished, this, finishInitialization);
    if (job->isFinished())
        finishInitialization();
}

XmppStorage::~XmppStorage() { shutdown(); }

void XmppStorage::shutdown()
{
    if (workerThread_.isRunning()) {
        QMetaObject::invokeMethod(worker_, [this]() { worker_->shutdown(); }, Qt::BlockingQueuedConnection);
    }
    workerThread_.quit();
    workerThread_.wait();
}

XmppConfig XmppStorage::readConfig() const
{
    QSettings  settings;
    XmppConfig config;
    config.originId = settings.value(QStringLiteral("storage.xmpppubsub.originId")).toString();
    if (config.originId.isEmpty()) {
        config.originId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        settings.setValue(QStringLiteral("storage.xmpppubsub.originId"), config.originId);
    }

    config.jid
        = settings.value(QStringLiteral("storage.xmpppubsub.jid")).toString().trimmed().section(QLatin1Char('/'), 0, 0);
    config.password = settings.value(QStringLiteral("storage.xmpppubsub.password")).toString();
    config.host     = settings.value(QStringLiteral("storage.xmpppubsub.host")).toString();
    config.port     = settings.value(QStringLiteral("storage.xmpppubsub.port"), 0).toInt();

    const QString defaultResource = QStringLiteral("QtNote-") + config.originId.left(8);
    config.resource = settings.value(QStringLiteral("storage.xmpppubsub.resource"), defaultResource).toString();
    config.nodeName
        = settings.value(QStringLiteral("storage.xmpppubsub.node"), QStringLiteral("urn:xmpp:qtnote:notes")).toString();
    config.timeoutMs = settings.value(QStringLiteral("storage.xmpppubsub.timeoutMs"), 15000).toInt();
    if (!config.jid.isEmpty()) {
        const auto key = SecureKeyStore::read(storageKeyName(config.jid));
        if (key)
            config.masterKey = key.value;
        const auto omemoKey
            = SecureKeyStore::loadOrCreate(QStringLiteral("xmpp-omemo-state-key-v1:%1").arg(config.jid));
        if (omemoKey)
            config.omemoStateKey = omemoKey.value;
        const auto accountHash = QCryptographicHash::hash(config.jid.toUtf8(), QCryptographicHash::Sha256).toHex();
        config.omemoStatePath  = Utils::qtnoteDataDir() + QStringLiteral("/xmpp-omemo-")
            + QString::fromLatin1(accountHash.left(16)) + QStringLiteral(".state");
    }
    return config;
}

bool XmppStorage::configIsValid(const XmppConfig &config, QString *error) const
{
    const QString bareJid = config.jid.section(QLatin1Char('/'), 0, 0);
    const int     at      = bareJid.indexOf(QLatin1Char('@'));
    if (at <= 0 || at == bareJid.size() - 1) {
        if (error) {
            *error = tr("Enter a valid XMPP JID such as user@example.org.");
        }
        return false;
    }
    if (config.password.isEmpty()) {
        if (error) {
            *error = tr("Enter the XMPP account password.");
        }
        return false;
    }
    if (config.resource.isEmpty()) {
        if (error) {
            *error = tr("Enter an XMPP resource name.");
        }
        return false;
    }
    if (config.nodeName.isEmpty()) {
        if (error) {
            *error = tr("Enter a PubSub node name.");
        }
        return false;
    }
    if (config.masterKey.size() != SecureEnvelope::MasterKeySize) {
        if (error)
            *error = tr("Create or import the XMPP storage encryption key.");
        return false;
    }
    if (config.omemoStateKey.size() != SecureEnvelope::MasterKeySize || config.omemoStatePath.isEmpty()) {
        if (error)
            *error = tr("The local OMEMO state cannot be protected by the system keychain.");
        return false;
    }
    return true;
}

bool XmppStorage::init()
{
    if (errorState_) {
        return false;
    }

    config_ = readConfig();

    QString validationError;
    if (!configIsValid(config_, &validationError)) {
        accessible_ = false;
        cacheValid_ = false;
        return false;
    }

    const auto result = invokeWorker<XmppStatusResult>(worker_, [&]() {
        worker_->setConfig(config_);
        return worker_->probe();
    });

    accessible_ = result.ok;
    cacheValid_ = false;
    if (!result.ok) {
        enterErrorState(result.error, true);
    }
    return result.ok;
}

StorageInitJob *XmppStorage::initAsync(QObject *owner)
{
    auto *job = new StorageInitJob(owner ? owner : this);
    job->start();
    if (errorState_) {
        job->fail({ StorageError::Unavailable, errorStateMessage_, false });
        return job;
    }
    config_ = readConfig();
    QString validationError;
    if (!configIsValid(config_, &validationError)) {
        job->fail({ StorageError::NotConfigured, validationError, false });
        return job;
    }
    const auto               config = config_;
    QPointer<StorageInitJob> guard(job);
    QMetaObject::invokeMethod(
        worker_,
        [this, guard, config]() {
            worker_->setConfig(config);
            const auto result = worker_->probe();
            QMetaObject::invokeMethod(
                this,
                [this, guard, result]() {
                    if (!guard || guard->isFinished())
                        return;
                    accessible_ = result.ok;
                    cacheValid_ = false;
                    if (result.ok)
                        guard->complete();
                    else {
                        enterErrorState(result.error, true);
                        guard->fail({ StorageError::Network, result.error, false });
                    }
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
    return job;
}

const QString XmppStorage::systemName() const { return storageId; }

const QString XmppStorage::name() const { return tr("XMPP Private Notes"); }

QIcon XmppStorage::storageIcon() const
{
    return themedIcon(QStringLiteral("im-jabber"), QStringLiteral("network-server"));
}

QIcon XmppStorage::noteIcon() const
{
    return themedIcon(QStringLiteral("document-edit"), QStringLiteral("text-x-generic"));
}

bool XmppStorage::isAccessible() const { return accessible_; }

QList<Note::Format> XmppStorage::availableFormats() const { return { Note::Markdown }; }

Note XmppStorage::fromRemote(const XmppRemoteNote &remote)
{
    Note note(new NoteData(this));
    applyRemote(note, remote);
    return note;
}

void XmppStorage::applyRemote(Note &note, const XmppRemoteNote &remote)
{
    note.setId(remote.id);
    note.setTitle(remote.title);
    note.setFormat(Note::Markdown);
    note.setLastChangeUTC(remote.modified);
    note.setTags(remote.tags);
    note.setBackendValue(QStringLiteral("revision"), remote.revision);
    note.setBackendValue(QStringLiteral("parentRevision"), remote.parentRevision);
    note.setBackendValue(QStringLiteral("originId"), remote.originId);
    if (remote.contentPresent)
        note.setText(remote.content, Note::Markdown);
    else
        note.unload();
}

XmppRemoteNote XmppStorage::toRemote(const Note &note) const
{
    XmppRemoteNote remote;
    remote.id             = note.id();
    remote.revision       = note.backendValue(QStringLiteral("revision")).toString();
    remote.parentRevision = note.backendValue(QStringLiteral("parentRevision")).toString();
    remote.originId       = note.backendValue(QStringLiteral("originId")).toString();
    remote.title          = note.title();
    remote.content        = note.text();
    remote.modified       = note.lastChangeUTC();
    remote.format         = QStringLiteral("markdown");
    remote.tags           = note.tags();
    remote.contentPresent = note.isLoaded();
    return remote;
}

QList<Note> XmppStorage::noteList(int limit)
{
    if (cacheValid_) {
        auto notes = cache_.values();
        std::sort(notes.begin(), notes.end(), noteListItemModifyComparer);
        return limit > 0 ? notes.mid(0, limit) : notes;
    }
    if (errorState_) {
        auto cached = cache_.values();
        std::sort(cached.begin(), cached.end(), noteListItemModifyComparer);
        return limit > 0 ? cached.mid(0, limit) : cached;
    }
    if (!accessible_ && !init()) {
        return {};
    }

    const auto result = invokeWorker<XmppListResult>(worker_, [&]() { return worker_->listNotes(); });

    if (!result.ok) {
        reportError(result.error);
        auto cached = cache_.values();
        std::sort(cached.begin(), cached.end(), noteListItemModifyComparer);
        return limit > 0 ? cached.mid(0, limit) : cached;
    }

    QHash<QString, Note> refreshed;
    refreshed.reserve(result.notes.size());
    for (const auto &remote : result.notes) {
        const auto old = cache_.constFind(remote.id);
        if (old != cache_.cend()) {
            if (old.value().backendValue(QStringLiteral("revision")).toString() == remote.revision) {
                refreshed.insert(remote.id, old.value());
                continue;
            }
        }
        refreshed.insert(remote.id, fromRemote(remote));
    }

    cache_      = std::move(refreshed);
    cacheValid_ = true;
    accessible_ = true;

    if (result.partial) {
        reportError(tr("The XMPP server returned only one paginated PubSub page. "
                       "Some notes may be missing because this server does not expose item IDs."));
    }

    auto notes = cache_.values();
    std::sort(notes.begin(), notes.end(), noteListItemModifyComparer);
    return limit > 0 ? notes.mid(0, limit) : notes;
}

NoteListJob *XmppStorage::refreshNotesAsync(int limit, QObject *owner)
{
    auto *job = new NoteListJob(owner ? owner : this);
    job->start();
    if (cacheValid_) {
        auto notes = cache_.values();
        std::sort(notes.begin(), notes.end(), noteListItemModifyComparer);
        QPointer<NoteListJob> guard(job);
        QTimer::singleShot(0, this, [guard, notes = limit > 0 ? notes.mid(0, limit) : notes]() {
            if (guard && !guard->isFinished())
                guard->complete(notes);
        });
        return job;
    }
    if (errorState_) {
        job->fail({ StorageError::Unavailable, errorStateMessage_, false });
        return job;
    }
    const auto            config = readConfig();
    QPointer<NoteListJob> guard(job);
    QMetaObject::invokeMethod(
        worker_,
        [this, guard, config, limit]() {
            worker_->setConfig(config);
            auto           status = worker_->probe();
            XmppListResult result;
            if (status.ok)
                result = worker_->listNotes();
            else {
                result.ok    = false;
                result.error = status.error;
            }
            QMetaObject::invokeMethod(
                this,
                [this, guard, result, limit]() {
                    if (!guard || guard->isFinished())
                        return;
                    if (!result.ok) {
                        enterErrorState(result.error, true);
                        guard->fail({ StorageError::Network, result.error, false });
                        return;
                    }
                    QHash<QString, Note> refreshed;
                    for (const auto &remote : result.notes) {
                        const auto old = cache_.constFind(remote.id);
                        if (old != cache_.cend()
                            && old.value().backendValue(QStringLiteral("revision")).toString() == remote.revision)
                            refreshed.insert(remote.id, old.value());
                        else
                            refreshed.insert(remote.id, fromRemote(remote));
                    }
                    cache_      = std::move(refreshed);
                    cacheValid_ = accessible_ = true;
                    auto notes                = cache_.values();
                    std::sort(notes.begin(), notes.end(), noteListItemModifyComparer);
                    guard->complete(limit > 0 ? notes.mid(0, limit) : notes);
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
    return job;
}

Note XmppStorage::note(const QString &id)
{
    if (id.isEmpty()) {
        return {};
    }
    if (errorState_) {
        return cache_.value(id);
    }
    if (!accessible_ && !init()) {
        return {};
    }

    const auto result = invokeWorker<XmppNoteResult>(worker_, [&]() { return worker_->getNote(id); });
    if (!result.ok) {
        if (result.notFound) {
            cache_.remove(id);
            return {};
        }
        reportError(result.error);
        return {};
    }

    auto remote = fromRemote(result.note);
    cache_.insert(id, remote);
    accessible_ = true;
    return remote;
}

NoteLoadJob *XmppStorage::loadNoteAsync(const QString &id, QObject *owner)
{
    auto *job = new NoteLoadJob(owner ? owner : this);
    job->start();
    if (id.isEmpty() || errorState_) {
        job->fail({ id.isEmpty() ? StorageError::NotFound : StorageError::Unavailable,
                    id.isEmpty() ? tr("Note was not found") : errorStateMessage_, false });
        return job;
    }
    const auto            config = readConfig();
    QPointer<NoteLoadJob> guard(job);
    QMetaObject::invokeMethod(
        worker_,
        [this, guard, config, id]() {
            worker_->setConfig(config);
            const auto result = worker_->getNote(id);
            QMetaObject::invokeMethod(
                this,
                [this, guard, result, id]() {
                    if (!guard || guard->isFinished())
                        return;
                    if (!result.ok) {
                        if (result.notFound)
                            cache_.remove(id);
                        guard->fail(
                            { result.notFound ? StorageError::NotFound : StorageError::Network, result.error, false });
                        return;
                    }
                    auto loaded = fromRemote(result.note);
                    cache_.insert(id, loaded);
                    accessible_ = true;
                    guard->complete(loaded);
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
    return job;
}

Note XmppStorage::createNote()
{
    Note note(new NoteData(this));
    note.setText(QString(), Note::Markdown);
    note.setLastChangeUTC(QDateTime::currentDateTimeUtc());
    return note;
}

bool XmppStorage::loadNote(Note &note)
{
    const QString id = note.id();
    if (id.isEmpty()) {
        return true;
    }
    if (errorState_) {
        return false;
    }
    if (!accessible_ && !init()) {
        return false;
    }

    const auto result = invokeWorker<XmppNoteResult>(worker_, [&]() { return worker_->getNote(id); });
    if (!result.ok) {
        reportError(result.error);
        return false;
    }

    applyRemote(note, result.note);
    accessible_ = true;
    return true;
}

bool XmppStorage::saveNote(const Note &note)
{
    if (note.isNull()) {
        return false;
    }

    if (note.storage() != this) {
        reportError(tr("Attempted to save a note owned by another storage."));
        return false;
    }

    Note saved = note;
    if (!saved.id().isEmpty() && !saved.isLoaded() && !loadNote(saved)) {
        return false;
    }
    if (errorState_) {
        return false;
    }
    if (!accessible_ && !init()) {
        return false;
    }

    const QString oldId  = note.id();
    const auto    local  = toRemote(saved);
    const auto    result = invokeWorker<XmppNoteResult>(worker_, [&]() { return worker_->saveNote(local); });

    if (!result.ok) {
        if (result.conflict) {
            if (result.remoteOnConflict) {
                cache_.insert(result.remoteOnConflict->id, fromRemote(*result.remoteOnConflict));
            }
            reportError(tr("Save conflict: the note changed on another XMPP resource. "
                           "The local text was preserved and the server item was not overwritten."),
                        true);
        } else {
            reportError(result.error);
        }
        return false;
    }

    applyRemote(saved, result.note);
    const QString newId   = saved.id();
    const bool    existed = !oldId.isEmpty() && cache_.contains(oldId);

    if (!oldId.isEmpty() && oldId != newId) {
        cache_.remove(oldId);
    }
    cache_.insert(newId, saved);
    cacheValid_ = true;
    accessible_ = true;

    if (!oldId.isEmpty() && oldId != newId) {
        emit noteIdChanged(saved, oldId);
    }
    if (existed || !oldId.isEmpty()) {
        emit noteModified(saved);
    } else {
        emit noteAdded(saved);
    }
    return true;
}

NoteSaveJob *XmppStorage::saveNoteAsync(const Note &note, QObject *owner)
{
    auto *job = new NoteSaveJob(owner ? owner : this);
    job->start();
    if (note.isNull() || note.storage() != this || !note.isLoaded() || errorState_) {
        job->fail({ StorageError::Other,
                    errorState_ ? errorStateMessage_ : tr("The note cannot be saved in its current state."), false });
        return job;
    }
    const auto            config = readConfig();
    const auto            local  = toRemote(note);
    const auto            oldId  = note.id();
    QPointer<NoteSaveJob> guard(job);
    QMetaObject::invokeMethod(
        worker_,
        [this, guard, config, local, oldId]() {
            worker_->setConfig(config);
            const auto result = worker_->saveNote(local);
            QMetaObject::invokeMethod(
                this,
                [this, guard, result, oldId]() {
                    if (!guard || guard->isFinished())
                        return;
                    if (!result.ok) {
                        if (result.remoteOnConflict)
                            cache_.insert(result.remoteOnConflict->id, fromRemote(*result.remoteOnConflict));
                        guard->fail(
                            { result.conflict ? StorageError::Conflict : StorageError::Network, result.error, false });
                        return;
                    }
                    auto       saved   = fromRemote(result.note);
                    const bool existed = !oldId.isEmpty() && cache_.contains(oldId);
                    if (!oldId.isEmpty() && oldId != saved.id())
                        cache_.remove(oldId);
                    cache_.insert(saved.id(), saved);
                    cacheValid_ = accessible_ = true;
                    if (!oldId.isEmpty() && oldId != saved.id())
                        emit noteIdChanged(saved, oldId);
                    if (existed || !oldId.isEmpty())
                        emit noteModified(saved);
                    else
                        emit noteAdded(saved);
                    guard->complete(saved);
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
    return job;
}

void XmppStorage::removeNote(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return;
    }
    if (errorState_) {
        return;
    }
    if (!accessible_ && !init()) {
        return;
    }

    Note removed = cache_.value(noteId);
    if (removed.isNull()) {
        removed = note(noteId);
    }

    const auto result = invokeWorker<XmppStatusResult>(worker_, [&]() { return worker_->deleteNote(noteId); });
    if (!result.ok && !result.notFound) {
        reportError(result.error);
        return;
    }

    cache_.remove(noteId);
    if (!removed.isNull()) {
        emit noteRemoved(removed);
    }
}

NoteRemoveJob *XmppStorage::removeNoteAsync(const QString &noteId, QObject *owner)
{
    auto *job = new NoteRemoveJob(owner ? owner : this);
    job->start();
    if (noteId.isEmpty() || errorState_) {
        job->fail({ noteId.isEmpty() ? StorageError::NotFound : StorageError::Unavailable,
                    noteId.isEmpty() ? tr("Note was not found") : errorStateMessage_, false });
        return job;
    }
    const auto              config  = readConfig();
    const auto              removed = cache_.value(noteId);
    QPointer<NoteRemoveJob> guard(job);
    QMetaObject::invokeMethod(
        worker_,
        [this, guard, config, noteId, removed]() {
            worker_->setConfig(config);
            const auto result = worker_->deleteNote(noteId);
            QMetaObject::invokeMethod(
                this,
                [this, guard, result, noteId, removed]() {
                    if (!guard || guard->isFinished())
                        return;
                    if (!result.ok && !result.notFound) {
                        guard->fail({ StorageError::Network, result.error, false });
                        return;
                    }
                    cache_.remove(noteId);
                    if (!removed.isNull())
                        emit noteRemoved(removed);
                    guard->complete();
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
    return job;
}

void XmppStorage::onRemoteNotePublished(const XmppRemoteNote &remote)
{
    if (errorState_) {
        return;
    }
    if (remote.id.isEmpty()) {
        return;
    }

    const auto previous = cache_.value(remote.id);
    QString    previousRevision;
    QString    previousParentRevision;
    if (!previous.isNull()) {
        previousRevision       = previous.backendValue(QStringLiteral("revision")).toString();
        previousParentRevision = previous.backendValue(QStringLiteral("parentRevision")).toString();
    }

    if (!previous.isNull() && previousRevision == remote.revision) {
        return;
    }

    const bool siblingConflict = !previous.isNull() && !previousParentRevision.isEmpty()
        && previousParentRevision == remote.parentRevision && previousRevision != remote.revision;

    auto incoming = fromRemote(remote);
    cache_.insert(remote.id, incoming);
    cacheValid_ = true;

    if (previous.isNull()) {
        emit noteAdded(incoming);
    } else {
        emit noteModified(incoming);
    }

    if (siblingConflict) {
        reportError(tr("Parallel XMPP note revisions were detected. "
                       "The latest server item is displayed; an automatic merge is not available."),
                    true);
    }
}

void XmppStorage::onRemoteNoteRetracted(const QString &id)
{
    if (errorState_) {
        return;
    }
    const auto removed = cache_.take(id);
    if (!removed.isNull()) {
        emit noteRemoved(removed);
    } else {
        emit invalidated();
    }
}

void XmppStorage::onRemoteNodeInvalidated()
{
    if (errorState_) {
        return;
    }
    cache_.clear();
    cacheValid_ = false;
    emit invalidated();
}

void XmppStorage::onConnectionChanged(bool connected)
{
    if (errorState_) {
        return;
    }
    accessible_ = connected;
    cacheValid_ = false;
    emit invalidated();
}

void XmppStorage::reportError(const QString &error, bool invalidate)
{
    if (!error.isEmpty() && error != lastReportedError_) {
        lastReportedError_ = error;
        emit storageErorr(tr("XMPP private notes error: %1").arg(error));
    }
    if (invalidate) {
        cacheValid_ = false;
        emit invalidated();
    }
}

void XmppStorage::enterErrorState(const QString &error, bool invalidate)
{
    if (errorState_ && error == errorStateMessage_) {
        return;
    }

    errorState_        = true;
    errorStateMessage_ = error;
    accessible_        = false;
    cacheValid_        = false;

    if (workerThread_.isRunning()) {
        QMetaObject::invokeMethod(worker_, [this]() { worker_->shutdown(); }, Qt::BlockingQueuedConnection);
    }

    reportError(error, invalidate);
}

void XmppStorage::clearErrorState()
{
    errorState_ = false;
    errorStateMessage_.clear();
    lastReportedError_.clear();
}

void XmppStorage::applyConfig(const XmppConfig &config)
{
    QSettings settings;
    settings.setValue(QStringLiteral("storage.xmpppubsub.jid"), config.jid);
    settings.setValue(QStringLiteral("storage.xmpppubsub.password"), config.password);
    settings.setValue(QStringLiteral("storage.xmpppubsub.host"), config.host);
    settings.setValue(QStringLiteral("storage.xmpppubsub.port"), config.port);
    settings.setValue(QStringLiteral("storage.xmpppubsub.resource"), config.resource);
    settings.setValue(QStringLiteral("storage.xmpppubsub.node"), config.nodeName);
    settings.setValue(QStringLiteral("storage.xmpppubsub.timeoutMs"), config.timeoutMs);
    settings.setValue(QStringLiteral("storage.xmpppubsub.originId"), config.originId);

    clearErrorState();
    cache_.clear();
    cacheValid_ = false;
    accessible_ = false;
    config_     = readConfig();
    init();
    emit invalidated();
}

QWidget *XmppStorage::settingsWidget()
{
    const auto current = readConfig();
    auto      *widget  = new XmppSettingsWidget(current);
    widget->setKeyState(SecureEnvelope::keyId(current.masterKey));
    connect(this, &XmppStorage::encryptionKeyChanged, widget, &XmppSettingsWidget::setKeyState);
    connect(widget, &XmppSettingsWidget::apply, this, [this, widget]() { applyConfig(widget->config()); });
    connect(widget, &XmppSettingsWidget::createKeyRequested, this, [this, widget](const QString &jid) {
        if (jid.isEmpty()) {
            widget->setKeyState({}, tr("Enter the XMPP JID first"));
            return;
        }
        auto existing = SecureKeyStore::read(storageKeyName(jid));
        if (existing) {
            widget->setKeyState(SecureEnvelope::keyId(existing.value), tr("A key already exists"));
            return;
        }
        const auto key   = SecureEnvelope::generateMasterKey();
        const auto error = SecureKeyStore::write(storageKeyName(jid), key);
        if (error) {
            widget->setKeyState({}, error.message);
            return;
        }
        widget->setKeyState(SecureEnvelope::keyId(key));
        clearErrorState();
    });
    connect(widget, &XmppSettingsWidget::importKeyRequested, this,
            [this, widget](const QString &jid, const QString &encoded) {
                if (jid.isEmpty()) {
                    widget->setKeyState({}, tr("Enter the XMPP JID first"));
                    return;
                }
                auto imported = SecureEnvelope::decodeRecoveryKey(encoded);
                if (!imported) {
                    widget->setKeyState({}, imported.error.message);
                    return;
                }
                auto existing = SecureKeyStore::read(storageKeyName(jid));
                if (existing && existing.value != imported.value) {
                    widget->setKeyState(SecureEnvelope::keyId(existing.value),
                                        tr("A different key already exists; it was not replaced"));
                    return;
                }
                const auto error = SecureKeyStore::write(storageKeyName(jid), imported.value);
                if (error) {
                    widget->setKeyState({}, error.message);
                    return;
                }
                widget->setKeyState(SecureEnvelope::keyId(imported.value));
                clearErrorState();
            });
    connect(widget, &XmppSettingsWidget::exportKeyRequested, this, [widget](const QString &jid) {
        auto key = SecureKeyStore::read(storageKeyName(jid));
        if (!key) {
            widget->setKeyState({}, key.error.message);
            return;
        }
        widget->setRecoveryKey(SecureEnvelope::encodeRecoveryKey(key.value));
        widget->setKeyState(SecureEnvelope::keyId(key.value));
    });
    connect(widget, &XmppSettingsWidget::omemoSyncRequested, this, [this, widget](const QString &jid) {
        auto config = readConfig();
        config.jid  = jid;
        QPointer<XmppSettingsWidget> guard(widget);
        QMetaObject::invokeMethod(worker_, [this, guard, config]() {
            worker_->setConfig(config);
            const auto result = worker_->requestStorageKey();
            QMetaObject::invokeMethod(this, [guard, result]() {
                if (!guard)
                    return;
                guard->setKeyState(
                    {}, result.ok ? XmppSettingsWidget::tr("Storage-key request sent via OMEMO") : result.error);
            });
        });
    });
    connect(widget, &XmppSettingsWidget::omemoDevicesRequested, this, [this, widget](const QString &jid) {
        auto config = readConfig();
        config.jid  = jid;
        QPointer<XmppSettingsWidget> guard(widget);
        QMetaObject::invokeMethod(worker_, [this, guard, config]() {
            worker_->setConfig(config);
            QString    error;
            const auto devices = worker_->ownOmemoDevices(&error);
            QMetaObject::invokeMethod(this, [guard, devices, error]() {
                if (guard)
                    guard->setOmemoDevices(devices, error);
            });
        });
    });
    connect(widget, &XmppSettingsWidget::trustOmemoDeviceRequested, this,
            [this, widget](const QString &jid, const QByteArray &keyId) {
                auto config = readConfig();
                config.jid  = jid;
                QPointer<XmppSettingsWidget> guard(widget);
                QMetaObject::invokeMethod(worker_, [this, guard, config, keyId]() {
                    worker_->setConfig(config);
                    const auto result = worker_->trustOwnOmemoDevice(keyId);
                    QMetaObject::invokeMethod(this, [guard, result]() {
                        if (guard)
                            guard->setKeyState(
                                {}, result.ok ? XmppSettingsWidget::tr("OMEMO device trusted") : result.error);
                    });
                });
            });
    if (!current.jid.isEmpty()) {
        QTimer::singleShot(0, widget, [widget, jid = current.jid]() { emit widget->omemoDevicesRequested(jid); });
    }
    return widget;
}

QString XmppStorage::tooltip()
{
    if (errorState_) {
        return tr("XMPP private notes is stopped after an error:\n%1\n\nOpen storage settings and apply the "
                  "configuration to retry.")
            .arg(errorStateMessage_);
    }
    if (config_.jid.isEmpty()) {
        config_ = readConfig();
    }
    if (config_.jid.isEmpty()) {
        return tr("XMPP private notes is not configured.");
    }
    return tr("Account: %1\nPEP nodes: %2\nEncryption: end-to-end, key %3")
        .arg(config_.jid, config_.nodeName,
             QString::fromLatin1(SecureEnvelope::keyId(config_.masterKey).left(8).toHex()));
}

QString XmppStorage::storageId = QStringLiteral("xmpp-pubsub");

} // namespace QtNote
