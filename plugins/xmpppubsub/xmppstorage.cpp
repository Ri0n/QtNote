#include "xmppstorage.h"

#include "draftmanager.h"
#include "notedata.h"
#include "securekeystore.h"
#include "utils.h"
#include "xmppbackend.h"
#include "xmppkeyresolutiondialog.h"
#include "xmppsettingswidget.h"
#include "xmppworker.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QMetaObject>
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
#include <QNetworkInformation>
#endif
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QUuid>

#include <algorithm>
#include <memory>
#include <utility>

namespace QtNote {

namespace {
    constexpr int MinimumRetryDelaySeconds = 30;
    constexpr int MaximumRetryDelaySeconds = 300;
    const QString QtNoteKeychainService    = QStringLiteral("com.github.ri0n.qtnote");
    const QString PsiKeychainService       = QStringLiteral("xmpp");

    QString passwordKeyName(const QString &jid)
    {
        return QStringLiteral("xmpp-password-v1:%1").arg(jid.trimmed().section(QLatin1Char('/'), 0, 0));
    }

    QString storageKeyName(const QString &jid)
    {
        return QStringLiteral("xmpp-storage-master-key-v1:%1").arg(jid.trimmed().section(QLatin1Char('/'), 0, 0));
    }

    QIcon themedIcon(const QString &primary, const QString &fallback)
    {
        auto icon = QIcon::fromTheme(primary);
        if (icon.isNull()) {
            icon = QIcon::fromTheme(fallback);
        }
        return icon;
    }

    StorageError storageError(const XmppStatusResult &result, StorageError::Code fallback)
    {
        auto code = fallback;
        if (result.errorKind == XmppErrorKind::Authentication)
            code = StorageError::Authentication;
        else if (result.errorKind == XmppErrorKind::Configuration || result.errorKind == XmppErrorKind::Security)
            code = StorageError::Unavailable;
        return { code, result.error, result.retryable() };
    }

} // namespace

XmppStorage::XmppStorage(QObject *parent, XmppBackend *backend) : NoteStorage(parent)
{
    backend_ = backend ? backend : new XmppWorker;
    backend_->setParent(this);
    connect(backend_, &XmppBackend::remoteNotePublished, this, &XmppStorage::onRemoteNotePublished);
    connect(backend_, &XmppBackend::remoteNoteRetracted, this, &XmppStorage::onRemoteNoteRetracted);
    connect(backend_, &XmppBackend::remoteNodeInvalidated, this, &XmppStorage::onRemoteNodeInvalidated);
    connect(backend_, &XmppBackend::connectionChanged, this, &XmppStorage::onConnectionChanged);
    connect(backend_, &XmppBackend::backendError, this, [this](const QString &error) {
        if (error.contains(QStringLiteral("storage key mismatch"), Qt::CaseInsensitive))
            enterErrorState(error, true);
        else
            reportError(error);
    });
    connect(backend_, &XmppBackend::keySyncTrustRequested, this,
            [this](const QString &requestId, const QByteArray &keyId) {
                QMessageBox dialog(QMessageBox::Question, tr("Trust an own QtNote device?"),
                                   tr("Another device on your XMPP account wants to synchronize the QtNote storage "
                                      "key."),
                                   QMessageBox::NoButton, QApplication::activeWindow());
                dialog.setInformativeText(
                    tr("OMEMO fingerprint: %1\n\nOnly approve this request if you recognize the device or can "
                       "compare this fingerprint on both devices.")
                        .arg(QString::fromLatin1(keyId.toHex())));
                auto *reject  = dialog.addButton(tr("Reject"), QMessageBox::RejectRole);
                auto *approve = dialog.addButton(tr("Trust and send key"), QMessageBox::AcceptRole);
                dialog.setDefaultButton(reject);
                dialog.exec();
                if (dialog.clickedButton() == approve) {
                    QMetaObject::invokeMethod(backend_,
                                              [this, requestId]() { backend_->approveKeySyncRequest(requestId); });
                }
            });

    retryTimer_ = new QTimer(this);
    retryTimer_->setSingleShot(true);
    connect(retryTimer_, &QTimer::timeout, this, &XmppStorage::retryInitialization);

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    QNetworkInformation::loadDefaultBackend();
    if (auto *network = QNetworkInformation::instance()) {
        connect(network, &QNetworkInformation::reachabilityChanged, this,
                [this](QNetworkInformation::Reachability reachability) {
                    if (reachability == QNetworkInformation::Reachability::Disconnected
                        || reachability == QNetworkInformation::Reachability::Unknown || errorState_) {
                        return;
                    }
                    if (retryTimer_->isActive())
                        retryTimer_->stop();
                    retryDelaySeconds_ = MinimumRetryDelaySeconds;
                    QTimer::singleShot(0, this, &XmppStorage::retryInitialization);
                });
    }
#endif
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

void XmppStorage::resolveStorageKeys(const QString &jid, XmppSettingsWidget *settings)
{
    if (jid.isEmpty() || keyResolutionInProgress_)
        return;
    keyResolutionInProgress_           = true;
    auto config                        = readConfig();
    config.jid                         = jid;
    const auto                   epoch = configEpoch_;
    QPointer<XmppSettingsWidget> settingsGuard(settings);
    QMetaObject::invokeMethod(backend_, [this, config, jid, settingsGuard, epoch]() {
        if (shuttingDown_ || epoch != configEpoch_) {
            keyResolutionInProgress_ = false;
            return;
        }
        backend_->setConfig(config);
        backend_->ownOmemoDevicesAsync([this, jid, settingsGuard, epoch](auto devices, QString deviceError) mutable {
            QMetaObject::invokeMethod(
                this,
                [this, devices = std::move(devices), deviceError = std::move(deviceError), jid, settingsGuard,
                 epoch]() {
                    if (shuttingDown_ || epoch != configEpoch_) {
                        keyResolutionInProgress_ = false;
                        return;
                    }
                    QWidget *parent
                        = settingsGuard ? static_cast<QWidget *>(settingsGuard.data()) : QApplication::activeWindow();
                    const bool localKeyMissing = readConfig().masterKey.size() != SecureEnvelope::MasterKeySize;
                    XmppKeyResolutionDialog dialog(
                        localKeyMissing, devices, deviceError,
                        [this](const QList<QByteArray> &keyIds, XmppKeyResolutionDialog::StatusCompletion completion) {
                            QMetaObject::invokeMethod(
                                backend_, [this, keyIds, completion = std::move(completion)]() mutable {
                                    backend_->trustOwnOmemoDevicesAsync(
                                        keyIds,
                                        [this, completion = std::move(completion)](XmppStatusResult result) mutable {
                                            QMetaObject::invokeMethod(
                                                this,
                                                [completion = std::move(completion), result]() mutable {
                                                    completion(result);
                                                },
                                                Qt::QueuedConnection);
                                        });
                                });
                        },
                        [this](XmppKeyResolutionDialog::AuditCompletion completion) {
                            QMetaObject::invokeMethod(backend_, [this, completion = std::move(completion)]() mutable {
                                backend_->auditStorageKeysAsync(
                                    [this, completion = std::move(completion)](XmppKeyAuditResult result) mutable {
                                        QMetaObject::invokeMethod(
                                            this,
                                            [completion = std::move(completion), result = std::move(result)]() mutable {
                                                completion(std::move(result));
                                            },
                                            Qt::QueuedConnection);
                                    });
                            });
                        },
                        [this](const QList<QByteArray> &keys, const QByteArray &canonical,
                               XmppKeyResolutionDialog::RekeyCompletion completion) {
                            QMetaObject::invokeMethod(
                                backend_, [this, keys, canonical, completion = std::move(completion)]() mutable {
                                    backend_->rekeyStorageAsync(
                                        keys, canonical,
                                        [this, completion = std::move(completion)](XmppRekeyResult result) mutable {
                                            QMetaObject::invokeMethod(
                                                this,
                                                [completion = std::move(completion),
                                                 result     = std::move(result)]() mutable {
                                                    completion(std::move(result));
                                                },
                                                Qt::QueuedConnection);
                                        });
                                });
                        },
                        parent);
                    const auto accepted      = dialog.exec() == QDialog::Accepted;
                    const auto rekeyed       = dialog.rekeyResult();
                    const auto canonical     = dialog.canonicalKey();
                    keyResolutionInProgress_ = false;
                    if (!accepted)
                        return;
                    if (!rekeyed.ok) {
                        if (settingsGuard)
                            settingsGuard->setKeyState(SecureEnvelope::keyId(canonical), rekeyed.error);
                        return;
                    }
                    installReceivedStorageKey(jid, canonical);
                    if (settingsGuard) {
                        settingsGuard->setKeyState(
                            SecureEnvelope::keyId(canonical),
                            tr("Recovery complete: %1 notes use the canonical key").arg(rekeyed.migrated));
                    }
                });
        });
    });
}

XmppStorage::~XmppStorage() { shutdown(); }

void XmppStorage::shutdown()
{
    if (shuttingDown_)
        return;
    shuttingDown_ = true;
    ++configEpoch_;
    if (retryTimer_)
        retryTimer_->stop();
    if (backend_)
        backend_->shutdown();
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
    if (!config.jid.isEmpty()) {
        const auto ownPassword = SecureKeyStore::readPassword(QtNoteKeychainService, passwordKeyName(config.jid));
        if (ownPassword) {
            config.password = ownPassword.value;
            settings.remove(QStringLiteral("storage.xmpppubsub.password"));
        } else {
            // Psi uses service "xmpp" and the bare JID as its key. Importing it
            // also keeps QtNote usable on keychain backends that restrict
            // cross-application access later.
            const auto psiPassword = SecureKeyStore::readPassword(PsiKeychainService, config.jid);
            if (psiPassword) {
                config.password = psiPassword.value;
                if (!SecureKeyStore::writePassword(QtNoteKeychainService, passwordKeyName(config.jid), config.password))
                    settings.remove(QStringLiteral("storage.xmpppubsub.password"));
            } else {
                const auto legacy = settings.value(QStringLiteral("storage.xmpppubsub.password")).toString();
                if (!legacy.isEmpty()) {
                    config.password = legacy;
                    if (!SecureKeyStore::writePassword(QtNoteKeychainService, passwordKeyName(config.jid), legacy))
                        settings.remove(QStringLiteral("storage.xmpppubsub.password"));
                }
            }
        }
    }
    config.host = settings.value(QStringLiteral("storage.xmpppubsub.host")).toString();
    config.port = settings.value(QStringLiteral("storage.xmpppubsub.port"), 0).toInt();

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
        accessible_           = false;
        cacheValid_           = false;
        auto recoverable      = config_;
        recoverable.masterKey = QByteArray(SecureEnvelope::MasterKeySize, '\0');
        QString otherError;
        if (config_.masterKey.size() != SecureEnvelope::MasterKeySize && configIsValid(recoverable, &otherError)) {
            QTimer::singleShot(0, this, [this, jid = config_.jid]() { resolveStorageKeys(jid); });
        }
        return false;
    }

    auto *job = initAsync(this);
    connect(job, &StorageJob::finished, job, &QObject::deleteLater);
    return accessible_;
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
        auto recoverable      = config_;
        recoverable.masterKey = QByteArray(SecureEnvelope::MasterKeySize, '\0');
        QString otherError;
        if (config_.masterKey.size() != SecureEnvelope::MasterKeySize && configIsValid(recoverable, &otherError)) {
            QTimer::singleShot(0, this, [this, jid = config_.jid]() { resolveStorageKeys(jid); });
        }
        job->fail({ StorageError::NotConfigured, validationError, false });
        return job;
    }
    const auto               config = config_;
    const auto               epoch  = configEpoch_;
    QPointer<StorageInitJob> guard(job);
    QMetaObject::invokeMethod(
        backend_,
        [this, guard, config, epoch]() {
            if (shuttingDown_ || epoch != configEpoch_) {
                if (guard)
                    guard->cancel();
                return;
            }
            backend_->setConfig(config);
            backend_->probeAsync([this, guard, epoch](XmppStatusResult result) {
                QMetaObject::invokeMethod(
                    this,
                    [this, guard, epoch, result = std::move(result)]() {
                        if (!guard || guard->isFinished())
                            return;
                        if (shuttingDown_ || epoch != configEpoch_) {
                            guard->cancel();
                            return;
                        }
                        accessible_ = result.ok;
                        cacheValid_ = false;
                        if (result.ok) {
                            resetRetryBackoff();
                            guard->complete();
                        } else {
                            if (result.retryable())
                                handleTransientFailure(result.error);
                            else
                                enterErrorState(result.error, true);
                            guard->fail(storageError(result, StorageError::Network));
                        }
                    },
                    Qt::QueuedConnection);
            });
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
    const auto            config = config_;
    const auto            epoch  = configEpoch_;
    QPointer<NoteListJob> guard(job);
    QMetaObject::invokeMethod(
        backend_,
        [this, guard, config, limit, epoch]() {
            if (shuttingDown_ || epoch != configEpoch_) {
                if (guard)
                    guard->cancel();
                return;
            }
            backend_->setConfig(config);
            backend_->listNotesAsync([this, guard, limit, epoch](XmppListResult result) {
                QMetaObject::invokeMethod(
                    this,
                    [this, guard, result = std::move(result), limit, epoch]() {
                        if (!guard || guard->isFinished())
                            return;
                        if (shuttingDown_ || epoch != configEpoch_) {
                            guard->cancel();
                            return;
                        }
                        if (!result.ok) {
                            if (result.retryable())
                                handleTransientFailure(result.error);
                            else
                                enterErrorState(result.error, true);
                            guard->fail(storageError(result, StorageError::Network));
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
                        resetRetryBackoff();
                        auto notes = cache_.values();
                        std::sort(notes.begin(), notes.end(), noteListItemModifyComparer);
                        guard->complete(limit > 0 ? notes.mid(0, limit) : notes);
                    },
                    Qt::QueuedConnection);
            });
        },
        Qt::QueuedConnection);
    return job;
}

Note XmppStorage::note(const QString &id) { return cache_.value(id); }

NoteLoadJob *XmppStorage::loadNoteAsync(const QString &id, QObject *owner)
{
    auto *job = new NoteLoadJob(owner ? owner : this);
    job->start();
    if (id.isEmpty() || errorState_) {
        job->fail({ id.isEmpty() ? StorageError::NotFound : StorageError::Unavailable,
                    id.isEmpty() ? tr("Note was not found") : errorStateMessage_, false });
        return job;
    }
    const auto            config = config_;
    const auto            epoch  = configEpoch_;
    QPointer<NoteLoadJob> guard(job);
    QMetaObject::invokeMethod(
        backend_,
        [this, guard, config, id, epoch]() {
            if (shuttingDown_ || epoch != configEpoch_) {
                if (guard)
                    guard->cancel();
                return;
            }
            backend_->setConfig(config);
            backend_->getNoteAsync(id, [this, guard, id, epoch](XmppNoteResult result) {
                QMetaObject::invokeMethod(
                    this,
                    [this, guard, result = std::move(result), id, epoch]() {
                        if (!guard || guard->isFinished())
                            return;
                        if (shuttingDown_ || epoch != configEpoch_) {
                            guard->cancel();
                            return;
                        }
                        if (!result.ok) {
                            if (result.notFound)
                                cache_.remove(id);
                            guard->fail(
                                storageError(result, result.notFound ? StorageError::NotFound : StorageError::Network));
                            return;
                        }
                        auto loaded = fromRemote(result.note);
                        cache_.insert(id, loaded);
                        accessible_ = true;
                        guard->complete(loaded);
                    },
                    Qt::QueuedConnection);
            });
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
    if (id.isEmpty())
        return true;
    const auto cached = cache_.value(id);
    if (cached.isNull() || !cached.isLoaded())
        return false;
    note = cached;
    return true;
}

bool XmppStorage::saveNote(const Note &note)
{
    if (note.isNull() || note.storage() != this || !note.isLoaded())
        return false;
    const auto draftId = QUuid::createUuid();
    auto       error   = DraftManager::instance()->saveEditing(draftId, note, note.title(), note.text(), note.format());
    if (!error)
        error = DraftManager::instance()->markReady(draftId);
    return !error;
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
    const auto            config = config_;
    const auto            epoch  = configEpoch_;
    const auto            local  = toRemote(note);
    const auto            oldId  = note.id();
    QPointer<NoteSaveJob> guard(job);
    QMetaObject::invokeMethod(
        backend_,
        [this, guard, config, local, oldId, epoch]() {
            if (shuttingDown_ || epoch != configEpoch_) {
                if (guard)
                    guard->cancel();
                return;
            }
            backend_->setConfig(config);
            backend_->saveNoteAsync(local, [this, guard, oldId, epoch](XmppNoteResult result) {
                QMetaObject::invokeMethod(
                    this,
                    [this, guard, result = std::move(result), oldId, epoch]() {
                        if (!guard || guard->isFinished())
                            return;
                        if (shuttingDown_ || epoch != configEpoch_) {
                            guard->cancel();
                            return;
                        }
                        if (!result.ok) {
                            if (result.remoteOnConflict)
                                cache_.insert(result.remoteOnConflict->id, fromRemote(*result.remoteOnConflict));
                            guard->fail(
                                storageError(result, result.conflict ? StorageError::Conflict : StorageError::Network));
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
            });
        },
        Qt::QueuedConnection);
    return job;
}

void XmppStorage::removeNote(const QString &noteId)
{
    if (!noteId.isEmpty())
        DraftManager::instance()->queueRemoval(systemName(), noteId);
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
    const auto              config  = config_;
    const auto              epoch   = configEpoch_;
    const auto              removed = cache_.value(noteId);
    QPointer<NoteRemoveJob> guard(job);
    QMetaObject::invokeMethod(
        backend_,
        [this, guard, config, noteId, removed, epoch]() {
            if (shuttingDown_ || epoch != configEpoch_) {
                if (guard)
                    guard->cancel();
                return;
            }
            backend_->setConfig(config);
            backend_->deleteNoteAsync(noteId, [this, guard, noteId, removed, epoch](XmppStatusResult result) {
                QMetaObject::invokeMethod(
                    this,
                    [this, guard, result = std::move(result), noteId, removed, epoch]() {
                        if (!guard || guard->isFinished())
                            return;
                        if (shuttingDown_ || epoch != configEpoch_) {
                            guard->cancel();
                            return;
                        }
                        if (!result.ok && !result.notFound) {
                            guard->fail(storageError(result, StorageError::Network));
                            return;
                        }
                        cache_.remove(noteId);
                        if (!removed.isNull())
                            emit noteRemoved(removed);
                        guard->complete();
                    },
                    Qt::QueuedConnection);
            });
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
    if (!connected) {
        scheduleRetry();
    } else if (retryTimer_ && retryTimer_->isActive()) {
        retryTimer_->stop();
        QTimer::singleShot(0, this, &XmppStorage::retryInitialization);
    }
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
    const bool keyMismatch = error.contains(QStringLiteral("storage key mismatch"), Qt::CaseInsensitive);
    if (errorState_ && error == errorStateMessage_) {
        if (keyMismatch)
            QTimer::singleShot(0, this, [this]() { resolveStorageKeys(config_.jid); });
        return;
    }

    errorState_        = true;
    errorStateMessage_ = error;
    accessible_        = false;
    cacheValid_        = false;
    if (retryTimer_)
        retryTimer_->stop();

    if (!keyMismatch)
        backend_->shutdown();

    if (keyMismatch) {
        if (invalidate) {
            cacheValid_ = false;
            emit invalidated();
        }
        QTimer::singleShot(0, this, [this]() { resolveStorageKeys(config_.jid); });
    } else {
        reportError(error, invalidate);
    }
}

void XmppStorage::clearErrorState()
{
    errorState_ = false;
    errorStateMessage_.clear();
    lastReportedError_.clear();
}

void XmppStorage::handleTransientFailure(const QString &error, bool invalidate)
{
    accessible_ = false;
    cacheValid_ = false;
    reportError(error, invalidate);
    scheduleRetry();
}

void XmppStorage::scheduleRetry()
{
    if (shuttingDown_ || errorState_ || retryInProgress_ || !retryTimer_ || retryTimer_->isActive())
        return;

    QString validationError;
    if (!configIsValid(config_, &validationError))
        return;

    const int delay    = retryDelaySeconds_;
    retryDelaySeconds_ = qMin(retryDelaySeconds_ * 2, MaximumRetryDelaySeconds);
    qInfo() << "XMPP storage reconnect scheduled in" << delay << "seconds";
    retryTimer_->start(delay * 1000);
}

void XmppStorage::retryInitialization()
{
    if (shuttingDown_ || errorState_ || retryInProgress_)
        return;

    QString validationError;
    if (!configIsValid(config_, &validationError))
        return;

    retryInProgress_    = true;
    auto      *job      = initAsync(this);
    const auto handled  = std::make_shared<bool>(false);
    const auto finished = [this, job, handled]() {
        if (std::exchange(*handled, true))
            return;
        retryInProgress_ = false;
        if (job->state() == StorageJob::Succeeded) {
            resetRetryBackoff();
            emit invalidated();
        } else if (job->error().retryable) {
            scheduleRetry();
        }
        job->deleteLater();
    };
    connect(job, &StorageJob::finished, this, finished);
    if (job->isFinished())
        finished();
}

void XmppStorage::resetRetryBackoff()
{
    if (retryTimer_)
        retryTimer_->stop();
    retryDelaySeconds_ = MinimumRetryDelaySeconds;
}

void XmppStorage::applyConfig(const XmppConfig &config)
{
    ++configEpoch_;
    shuttingDown_ = false;
    QSettings settings;
    settings.setValue(QStringLiteral("storage.xmpppubsub.jid"), config.jid);
    const auto passwordError
        = SecureKeyStore::writePassword(QtNoteKeychainService, passwordKeyName(config.jid), config.password);
    if (passwordError) {
        // Preserve compatibility on systems without a usable keychain. The
        // validation path will continue to report an empty password normally.
        settings.setValue(QStringLiteral("storage.xmpppubsub.password"), config.password);
        qWarning().noquote() << "Could not store XMPP password in the system keychain:" << passwordError.message;
    } else {
        settings.remove(QStringLiteral("storage.xmpppubsub.password"));
    }
    settings.setValue(QStringLiteral("storage.xmpppubsub.host"), config.host);
    settings.setValue(QStringLiteral("storage.xmpppubsub.port"), config.port);
    settings.setValue(QStringLiteral("storage.xmpppubsub.resource"), config.resource);
    settings.setValue(QStringLiteral("storage.xmpppubsub.node"), config.nodeName);
    settings.setValue(QStringLiteral("storage.xmpppubsub.timeoutMs"), config.timeoutMs);
    settings.setValue(QStringLiteral("storage.xmpppubsub.originId"), config.originId);

    clearErrorState();
    resetRetryBackoff();
    cache_.clear();
    cacheValid_ = false;
    accessible_ = false;
    config_     = readConfig();
    backend_->start();
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
        if (jid != config_.jid) {
            widget->setKeyState({}, tr("Apply the account settings before synchronizing the storage key"));
            return;
        }
        resolveStorageKeys(jid, widget);
    });
    connect(widget, &XmppSettingsWidget::omemoDevicesRequested, this, [this, widget](const QString &jid) {
        if (jid != config_.jid) {
            widget->setKeyState({}, tr("Apply the account settings before querying OMEMO devices"));
            return;
        }
        const auto                   config = config_;
        const auto                   epoch  = configEpoch_;
        QPointer<XmppSettingsWidget> guard(widget);
        QMetaObject::invokeMethod(backend_, [this, guard, config, epoch]() {
            if (shuttingDown_ || epoch != configEpoch_)
                return;
            backend_->setConfig(config);
            backend_->ownOmemoDevicesAsync([this, guard, epoch](auto devices, QString error) mutable {
                if (shuttingDown_ || epoch != configEpoch_)
                    return;
                const auto ownDevice = backend_->ownOmemoDevice();
                backend_->ownOmemoBundleValidAsync([this, guard, ownDevice, devices = std::move(devices),
                                                    error = std::move(error),
                                                    epoch](XmppStatusResult validity) mutable {
                    if (shuttingDown_ || epoch != configEpoch_)
                        return;
                    if (!validity.ok)
                        error = validity.error;
                    QMetaObject::invokeMethod(
                        this,
                        [guard, ownDevice, valid = validity.ok, devices = std::move(devices),
                         error = std::move(error)]() mutable {
                            if (guard)
                                guard->setOmemoDevices(ownDevice, valid, devices, error);
                        },
                        Qt::QueuedConnection);
                });
            });
        });
    });
    connect(widget, &XmppSettingsWidget::repairOmemoDeviceRequested, this, [this, widget](const QString &jid) {
        if (jid != config_.jid) {
            widget->setKeyState({}, tr("Apply the account settings before repairing the OMEMO device"));
            return;
        }
        const auto                   config = config_;
        const auto                   epoch  = configEpoch_;
        QPointer<XmppSettingsWidget> guard(widget);
        QMetaObject::invokeMethod(backend_, [this, guard, config, epoch]() {
            if (shuttingDown_ || epoch != configEpoch_)
                return;
            backend_->setConfig(config);
            backend_->repairOwnOmemoDeviceAsync([this, guard, epoch](XmppStatusResult result) {
                if (shuttingDown_ || epoch != configEpoch_)
                    return;
                QMetaObject::invokeMethod(
                    this,
                    [guard, result]() {
                        if (!guard)
                            return;
                        guard->setKeyState({},
                                           result.ok ? XmppSettingsWidget::tr("OMEMO device repaired") : result.error);
                        if (result.ok)
                            emit guard->omemoDevicesRequested(guard->config().jid);
                    },
                    Qt::QueuedConnection);
            });
        });
    });
    connect(widget, &XmppSettingsWidget::trustOmemoDeviceRequested, this,
            [this, widget](const QString &jid, const QByteArray &keyId) {
                if (jid != config_.jid) {
                    widget->setKeyState({}, tr("Apply the account settings before changing OMEMO trust"));
                    return;
                }
                const auto                   config = config_;
                const auto                   epoch  = configEpoch_;
                QPointer<XmppSettingsWidget> guard(widget);
                QMetaObject::invokeMethod(backend_, [this, guard, config, keyId, epoch]() {
                    if (shuttingDown_ || epoch != configEpoch_)
                        return;
                    backend_->setConfig(config);
                    backend_->trustOwnOmemoDeviceAsync(keyId, [this, guard, epoch](XmppStatusResult result) {
                        if (shuttingDown_ || epoch != configEpoch_)
                            return;
                        QMetaObject::invokeMethod(
                            this,
                            [guard, result]() {
                                if (guard)
                                    guard->setKeyState(
                                        {}, result.ok ? XmppSettingsWidget::tr("OMEMO device trusted") : result.error);
                            },
                            Qt::QueuedConnection);
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
