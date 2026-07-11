#include "xmppstorage.h"

#include "xmppdata.h"
#include "xmppsettingswidget.h"
#include "xmppworker.h"

#include <QMetaObject>
#include <QSettings>
#include <QUuid>

#include <algorithm>
#include <utility>

namespace QtNote {

namespace {

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
    connect(worker_, &XmppWorker::workerError, this, [this](const QString &error) {
        if (!errorState_) {
            enterErrorState(error, true);
        }
    });

    workerThread_.start();
}

XmppStorage::~XmppStorage()
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
        = settings.value(QStringLiteral("storage.xmpppubsub.node"), QStringLiteral("urn:xmpp:qtnote:notes:0"))
              .toString();
    config.timeoutMs = settings.value(QStringLiteral("storage.xmpppubsub.timeoutMs"), 15000).toInt();
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
    auto *data = new XmppData(this);
    data->applyRemoteNote(remote);
    return Note(data);
}

QList<Note> XmppStorage::noteList(int limit)
{
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
            auto *data = dynamic_cast<XmppData *>(old.value().data());
            if (data && data->revision() == remote.revision) {
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

Note XmppStorage::createNote()
{
    auto *data = new XmppData(this);
    data->initializeNew();
    return Note(data);
}

bool XmppStorage::loadData(XmppData &data)
{
    const QString id = data.toRemoteNote().id;
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

    data.applyRemoteNote(result.note);
    accessible_ = true;
    return true;
}

bool XmppStorage::saveNote(const Note &note)
{
    if (note.isNull()) {
        return false;
    }

    auto *data = dynamic_cast<XmppData *>(note.data());
    if (!data) {
        reportError(tr("Attempted to save a note owned by another storage."));
        return false;
    }

    if (!note.id().isEmpty() && !note.isLoaded() && !data->load()) {
        return false;
    }
    if (errorState_) {
        return false;
    }
    if (!accessible_ && !init()) {
        return false;
    }

    const QString oldId  = note.id();
    const auto    local  = data->toRemoteNote();
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

    data->applyRemoteNote(result.note);
    const QString newId   = note.id();
    const bool    existed = !oldId.isEmpty() && cache_.contains(oldId);

    if (!oldId.isEmpty() && oldId != newId) {
        cache_.remove(oldId);
    }
    cache_.insert(newId, note);
    cacheValid_ = true;
    accessible_ = true;

    if (!oldId.isEmpty() && oldId != newId) {
        emit noteIdChanged(note, oldId);
    }
    if (existed || !oldId.isEmpty()) {
        emit noteModified(note);
    } else {
        emit noteAdded(note);
    }
    return true;
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

void XmppStorage::onRemoteNotePublished(const XmppRemoteNote &remote)
{
    if (errorState_) {
        return;
    }
    if (remote.id.isEmpty()) {
        return;
    }

    const auto previous     = cache_.value(remote.id);
    auto      *previousData = previous.isNull() ? nullptr : dynamic_cast<XmppData *>(previous.data());

    if (previousData && previousData->revision() == remote.revision) {
        return;
    }

    const bool siblingConflict = previousData && !previousData->parentRevision().isEmpty()
        && previousData->parentRevision() == remote.parentRevision && previousData->revision() != remote.revision;

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
    if (!error.isEmpty()) {
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
    config_     = config;
    init();
    emit invalidated();
}

QWidget *XmppStorage::settingsWidget()
{
    auto *widget = new XmppSettingsWidget(readConfig());
    connect(widget, &XmppSettingsWidget::apply, this, [this, widget]() { applyConfig(widget->config()); });
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
    return tr("Account: %1\nPEP node: %2\nEncryption: server-private, not E2E").arg(config_.jid, config_.nodeName);
}

QString XmppStorage::storageId = QStringLiteral("xmpp-pubsub");

} // namespace QtNote
