#ifndef DRAFTMANAGER_H
#define DRAFTMANAGER_H

#include "draftstore.h"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <memory>

namespace QtNote {

class ConflictResolver;
class FileDraftStore;
class NoteSaveJob;
class NoteStorage;
class StorageJob;
struct StorageError;

class QTNOTE_EXPORT DraftManager final : public QObject {
    Q_OBJECT
public:
    static DraftManager *instance();
    ~DraftManager() override;

    bool    initialize(QString *error = nullptr);
    bool    isReady() const { return bool(store_); }
    QString lastError() const { return lastError_; }

    DraftStoreError    saveEditing(const QUuid &draftId, const Note &note, const QString &title, const QString &body,
                                   Note::Format format);
    DraftStoreError    markReady(const QUuid &draftId);
    DraftStoreError    discard(const QUuid &draftId);
    DraftStoreError    queueRemoval(const QString &storageId, const QString &noteId);
    void               publishPending();
    QList<DraftRecord> recoverableDrafts() const;
    void               setConflictResolver(std::unique_ptr<ConflictResolver> resolver);
    /// Resolves a conflict discovered after a storage operation was acknowledged.
    void resolveConcurrentEdit(const Note &localVersion, const Note &remoteVersion, const QString &message);

signals:
    void draftPublished(const QUuid &draftId, const Note &note);
    void draftPublishFailed(const QUuid &draftId, const QString &message);
    void publishingIdle();
    void publicationAbandoned(const QString &message);
    void conflictResolved(const QString &message);

private:
    explicit DraftManager(QObject *parent = nullptr);
    QByteArray loadOrCreateMasterKey(QString *error);
    void       process(const DraftRecord &record);
    void       publish(const DraftRecord &record);
    void       remove(const DraftRecord &record);
    void       retry(const DraftRecord &record, const QString &message, bool retryable = true);
    void       resolveConflict(const DraftRecord &record, const StorageError &error, const Note &remoteNote = {});
    void       storageAboutToBeRemoved(NoteStorage *storage);

    std::unique_ptr<FileDraftStore>    store_;
    QSet<QUuid>                        publishing_;
    QHash<QUuid, QPointer<StorageJob>> publishJobs_;
    QString                            lastError_;
    std::unique_ptr<ConflictResolver>  conflictResolver_;
};

} // namespace QtNote

#endif // DRAFTMANAGER_H
