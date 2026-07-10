#include "nextcloudstorage.h"

#include "nextclouddata.h"
#include "nextcloudsettingswidget.h"
#include "nextcloudworker.h"

#include <QMetaObject>
#include <QSet>
#include <QSettings>
#include <QUrl>

#include <algorithm>
#include <utility>

namespace QtNote {

namespace {

    template <typename Result, typename Function> Result invokeWorker(NextcloudWorker *worker, Function &&function)
    {
        Result result;

        if (QThread::currentThread() == worker->thread()) {
            return function();
        }

        const bool invoked
            = QMetaObject::invokeMethod(worker, [&]() { result = function(); }, Qt::BlockingQueuedConnection);

        if (!invoked) {
            result.ok    = false;
            result.error = QStringLiteral("Failed to invoke the Nextcloud network worker");
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

NextcloudStorage::NextcloudStorage(QObject *parent) : NoteStorage(parent)
{
    workerThread_.setObjectName(QStringLiteral("QtNoteNextcloudWorker"));
    worker_ = new NextcloudWorker;
    worker_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    workerThread_.start();
}

NextcloudStorage::~NextcloudStorage()
{
    workerThread_.quit();
    workerThread_.wait();
}

NextcloudConfig NextcloudStorage::readConfig() const
{
    QSettings       settings;
    NextcloudConfig config;

    const auto urlText = settings.value(QStringLiteral("storage.nextcloud.url")).toString();
    if (!urlText.trimmed().isEmpty()) {
        config.serverUrl = QUrl::fromUserInput(urlText.trimmed());
    }

    config.userName    = settings.value(QStringLiteral("storage.nextcloud.username")).toString();
    config.appPassword = settings.value(QStringLiteral("storage.nextcloud.appPassword")).toString();
    config.timeoutMs   = settings.value(QStringLiteral("storage.nextcloud.timeoutMs"), 15000).toInt();

    return config;
}

bool NextcloudStorage::configIsValid(const NextcloudConfig &config, QString *error) const
{
    if (!config.serverUrl.isValid() || config.serverUrl.host().isEmpty()
        || (config.serverUrl.scheme() != QStringLiteral("https")
            && config.serverUrl.scheme() != QStringLiteral("http"))) {
        if (error) {
            *error = tr("Enter a valid HTTP or HTTPS Nextcloud server URL.");
        }
        return false;
    }

    if (config.userName.isEmpty()) {
        if (error) {
            *error = tr("Enter the Nextcloud user name.");
        }
        return false;
    }

    if (config.appPassword.isEmpty()) {
        if (error) {
            *error = tr("Enter a Nextcloud app password.");
        }
        return false;
    }

    return true;
}

bool NextcloudStorage::init()
{
    config_ = readConfig();

    QString validationError;
    if (!configIsValid(config_, &validationError)) {
        accessible_ = false;
        cacheValid_ = false;
        return false;
    }

    const auto result = invokeWorker<NextcloudStatusResult>(worker_, [&]() {
        worker_->setConfig(config_);
        return worker_->probe();
    });

    accessible_ = result.ok;
    cacheValid_ = false;

    if (!result.ok) {
        reportError(result.error);
    }
    return result.ok;
}

const QString NextcloudStorage::systemName() const { return storageId; }

const QString NextcloudStorage::name() const { return tr("Nextcloud Notes"); }

QIcon NextcloudStorage::storageIcon() const
{
    return themedIcon(QStringLiteral("folder-cloud"), QStringLiteral("network-server"));
}

QIcon NextcloudStorage::noteIcon() const
{
    return themedIcon(QStringLiteral("document-edit"), QStringLiteral("text-x-generic"));
}

bool NextcloudStorage::isAccessible() const { return accessible_; }

QList<Note::Format> NextcloudStorage::availableFormats() const { return { Note::Markdown }; }

Note NextcloudStorage::fromRemote(const NextcloudRemoteNote &remote)
{
    auto *data = new NextcloudData(this);
    data->applyServerNote(remote);
    return Note(data);
}

QList<Note> NextcloudStorage::noteList(int limit)
{
    if (!accessible_ && !init()) {
        return {};
    }

    const auto result = invokeWorker<NextcloudListResult>(worker_, [&]() { return worker_->listNotes(); });

    if (!result.ok) {
        reportError(result.error);
        auto cached = cache_.values();
        std::sort(cached.begin(), cached.end(), noteListItemModifyComparer);
        return limit > 0 ? cached.mid(0, limit) : cached;
    }

    accessible_ = true;
    QHash<QString, Note> refreshed;
    refreshed.reserve(result.notes.size());

    for (const auto &remote : result.notes) {
        const auto old = cache_.constFind(remote.id);
        if (old != cache_.cend()) {
            auto *oldData = dynamic_cast<NextcloudData *>(old.value().data());
            if (oldData && oldData->etag() == remote.etag) {
                refreshed.insert(remote.id, old.value());
                continue;
            }
        }
        refreshed.insert(remote.id, fromRemote(remote));
    }

    cache_      = std::move(refreshed);
    cacheValid_ = true;

    auto notes = cache_.values();
    std::sort(notes.begin(), notes.end(), noteListItemModifyComparer);
    return limit > 0 ? notes.mid(0, limit) : notes;
}

Note NextcloudStorage::note(const QString &id)
{
    if (id.isEmpty()) {
        return {};
    }
    if (!accessible_ && !init()) {
        return {};
    }

    const auto result = invokeWorker<NextcloudNoteResult>(worker_, [&]() { return worker_->getNote(id); });

    if (!result.ok) {
        if (result.httpStatus == 404) {
            cache_.remove(id);
            return {};
        }
        reportError(result.error);
        return {};
    }

    accessible_     = true;
    auto remoteNote = fromRemote(result.note);
    cache_.insert(result.note.id, remoteNote);
    return remoteNote;
}

Note NextcloudStorage::createNote()
{
    auto *data = new NextcloudData(this);
    data->initializeNew();
    return Note(data);
}

bool NextcloudStorage::loadData(NextcloudData &data)
{
    if (data.toRemoteNote().id.isEmpty()) {
        return true;
    }
    if (!accessible_ && !init()) {
        return false;
    }

    const QString id     = data.toRemoteNote().id;
    const auto    result = invokeWorker<NextcloudNoteResult>(worker_, [&]() { return worker_->getNote(id); });

    if (!result.ok) {
        reportError(result.error);
        return false;
    }

    data.applyServerNote(result.note);
    accessible_ = true;
    return true;
}

bool NextcloudStorage::saveNote(const Note &note)
{
    if (note.isNull()) {
        return false;
    }

    auto *data = dynamic_cast<NextcloudData *>(note.data());
    if (!data) {
        reportError(tr("Attempted to save a note owned by another storage."));
        return false;
    }

    if (!note.id().isEmpty() && !note.isLoaded() && !data->load()) {
        return false;
    }

    if (data->readOnly()) {
        reportError(tr("The note is read-only and cannot be saved."));
        return false;
    }

    if (!accessible_ && !init()) {
        return false;
    }

    const QString oldId = note.id();
    const auto    local = data->toRemoteNote();

    const auto result = invokeWorker<NextcloudNoteResult>(
        worker_, [&]() { return oldId.isEmpty() ? worker_->createNote(local) : worker_->updateNote(local); });

    if (!result.ok) {
        if (result.conflict) {
            if (result.remoteOnConflict) {
                cache_.insert(result.remoteOnConflict->id, fromRemote(*result.remoteOnConflict));
            }
            reportError(tr("Save conflict: this note was changed on the server. "
                           "Your local text was not overwritten. Reload the note or save the local text "
                           "as a new note."),
                        true);
        } else {
            reportError(result.error);
        }
        return false;
    }

    data->applyServerNote(result.note);
    accessible_ = true;

    const QString newId   = note.id();
    const bool    existed = !oldId.isEmpty() && cache_.contains(oldId);

    if (!oldId.isEmpty() && oldId != newId) {
        cache_.remove(oldId);
    }
    cache_.insert(newId, note);
    cacheValid_ = true;

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

void NextcloudStorage::removeNote(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return;
    }
    if (!accessible_ && !init()) {
        return;
    }

    Note removed = cache_.value(noteId);
    if (removed.isNull()) {
        removed = note(noteId);
    }

    const auto result = invokeWorker<NextcloudStatusResult>(worker_, [&]() { return worker_->deleteNote(noteId); });

    if (!result.ok && result.httpStatus != 404) {
        reportError(result.error);
        return;
    }

    cache_.remove(noteId);
    if (!removed.isNull()) {
        emit noteRemoved(removed);
    }
}

void NextcloudStorage::reportError(const QString &error, bool invalidate)
{
    if (!error.isEmpty()) {
        emit storageErorr(tr("Nextcloud Notes error: %1").arg(error));
    }
    if (invalidate) {
        cacheValid_ = false;
        emit invalidated();
    }
}

void NextcloudStorage::applyConfig(const NextcloudConfig &config)
{
    QSettings settings;
    settings.setValue(QStringLiteral("storage.nextcloud.url"),
                      config.serverUrl.toString(QUrl::RemoveQuery | QUrl::RemoveFragment));
    settings.setValue(QStringLiteral("storage.nextcloud.username"), config.userName);
    settings.setValue(QStringLiteral("storage.nextcloud.appPassword"), config.appPassword);
    settings.setValue(QStringLiteral("storage.nextcloud.timeoutMs"), config.timeoutMs);

    cache_.clear();
    cacheValid_ = false;
    accessible_ = false;
    config_     = config;
    init();
    emit invalidated();
}

QWidget *NextcloudStorage::settingsWidget()
{
    auto *widget = new NextcloudSettingsWidget(readConfig());
    connect(widget, &NextcloudSettingsWidget::apply, this, [this, widget]() { applyConfig(widget->config()); });
    return widget;
}

QString NextcloudStorage::tooltip()
{
    if (config_.serverUrl.isEmpty()) {
        config_ = readConfig();
    }

    if (config_.serverUrl.isEmpty()) {
        return tr("Nextcloud Notes is not configured.");
    }

    return tr("Server: %1\nUser: %2")
        .arg(config_.serverUrl.toDisplayString(QUrl::RemoveUserInfo | QUrl::RemoveQuery | QUrl::RemoveFragment),
             config_.userName);
}

QString NextcloudStorage::storageId = QStringLiteral("nextcloud");

} // namespace QtNote
