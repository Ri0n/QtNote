#include "draftmanager.h"

#include "filedraftstore.h"
#include "notedata.h"
#include "notemanager.h"
#include "notestorage.h"
#include "storagejob.h"
#include "utils.h"

#include <QApplication>
#include <QEventLoop>
#include <QTimer>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <qt5keychain/keychain.h>
#else
#include <qt6keychain/keychain.h>
#endif

namespace QtNote {
namespace {
    const QString KeychainService = QStringLiteral("com.github.ri0n.qtnote");
    const QString KeychainKey     = QStringLiteral("draft-store-master-key-v1");

    QString keychainError(QKeychain::Job *job)
    {
        return DraftManager::tr("Failed to access the secure keychain: %1").arg(job->errorString());
    }
} // namespace

DraftManager::DraftManager(QObject *parent) : QObject(parent) { }
DraftManager::~DraftManager() = default;

DraftManager *DraftManager::instance()
{
    static DraftManager *manager = new DraftManager(qApp);
    return manager;
}

QByteArray DraftManager::loadOrCreateMasterKey(QString *errorText)
{
    if (!QKeychain::isAvailable()) {
        *errorText = tr("No secure system keychain is available.");
        return {};
    }

    QKeychain::ReadPasswordJob read(KeychainService);
    read.setAutoDelete(false);
    read.setInsecureFallback(false);
    read.setKey(KeychainKey);
    QEventLoop loop;
    connect(&read, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    read.start();
    loop.exec();
    if (read.error() == QKeychain::NoError) {
        const auto key = read.binaryData();
        if (key.size() != FileDraftStore::MasterKeySize)
            *errorText = tr("The draft encryption key stored in the keychain is invalid.");
        return key.size() == FileDraftStore::MasterKeySize ? key : QByteArray();
    }
    if (read.error() != QKeychain::EntryNotFound) {
        *errorText = keychainError(&read);
        return {};
    }

    const auto key = FileDraftStore::generateMasterKey();
    if (key.size() != FileDraftStore::MasterKeySize) {
        *errorText = tr("A secure draft encryption key could not be generated.");
        return {};
    }
    QKeychain::WritePasswordJob write(KeychainService);
    write.setAutoDelete(false);
    write.setInsecureFallback(false);
    write.setKey(KeychainKey);
    write.setBinaryData(key);
    connect(&write, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    write.start();
    loop.exec();
    if (write.error() != QKeychain::NoError) {
        *errorText = keychainError(&write);
        return {};
    }
    return key;
}

bool DraftManager::initialize(QString *errorText)
{
    if (store_)
        return true;
    QString error;
    auto    key = loadOrCreateMasterKey(&error);
    if (key.isEmpty()) {
        lastError_ = error;
        if (errorText)
            *errorText = error;
        return false;
    }
    store_       = std::make_unique<FileDraftStore>(Utils::qtnoteDataDir() + QStringLiteral("/drafts"), std::move(key));
    auto records = store_->records();
    if (!records) {
        lastError_ = records.error.message;
        store_.reset();
        if (errorText)
            *errorText = lastError_;
        return false;
    }
    auto *notes = NoteManager::instance();
    connect(notes, &NoteManager::storageAboutToBeRemoved, this,
            [this](NoteStorage::Ptr storage) { storageAboutToBeRemoved(storage.data()); });
    connect(notes, &NoteManager::storageRemoved, this,
            [this](NoteStorage::Ptr) { QTimer::singleShot(0, this, &DraftManager::publishPending); });
    QTimer::singleShot(0, this, &DraftManager::publishPending);
    return true;
}

DraftStoreError DraftManager::saveEditing(const QUuid &draftId, const Note &note, const QString &title,
                                          const QString &body, Note::Format format)
{
    if (!store_)
        return { DraftStoreError::Locked, lastError_.isEmpty() ? tr("Draft store is locked") : lastError_ };
    DraftRecord record;
    record.id           = draftId;
    record.state        = DraftRecord::Editing;
    record.storageId    = note.storageId();
    record.remoteNoteId = note.id();
    record.title        = title;
    record.body         = body;
    record.format       = format;
    record.tags         = NoteData::tagsFromText(body);
    record.updatedAt    = QDateTime::currentDateTimeUtc();
    return store_->write(record);
}

DraftStoreError DraftManager::markReady(const QUuid &draftId)
{
    if (!store_)
        return { DraftStoreError::Locked, lastError_ };
    auto draft = store_->load(draftId);
    if (!draft)
        return draft.error;
    draft.value.state = draft.value.storageId.isEmpty() ? DraftRecord::NeedsRouting : DraftRecord::Ready;
    auto result       = store_->write(draft.value);
    if (!result)
        QTimer::singleShot(0, this, &DraftManager::publishPending);
    return result;
}

DraftStoreError DraftManager::discard(const QUuid &draftId)
{
    if (!store_)
        return { DraftStoreError::Locked, lastError_ };
    auto result = store_->remove(draftId);
    return result.code == DraftStoreError::NotFound ? DraftStoreError {} : result;
}

QList<DraftRecord> DraftManager::recoverableDrafts() const
{
    QList<DraftRecord> result;
    if (!store_)
        return result;
    auto records = store_->records();
    if (!records)
        return result;
    for (const auto &record : records.value) {
        if (record.state == DraftRecord::Editing)
            result.push_back(record);
    }
    return result;
}

void DraftManager::publishPending()
{
    if (!store_)
        return;
    auto records = store_->records();
    if (!records)
        return;
    for (const auto &record : records.value) {
        if (record.state == DraftRecord::NeedsRouting) {
            auto target = NoteManager::instance()->defaultStorage();
            if (!target)
                continue;
            auto routed      = record;
            routed.storageId = target->systemName();
            routed.remoteNoteId.clear();
            routed.state = DraftRecord::Ready;
            routed.lastError.clear();
            routed.retryAt = {};
            if (store_->write(routed))
                continue;
            publish(routed);
            continue;
        }
        if ((record.state == DraftRecord::Ready || record.state == DraftRecord::Retry
             || record.state == DraftRecord::Publishing)
            && !publishing_.contains(record.id)) {
            publish(record);
        }
    }
    if (publishing_.isEmpty())
        emit publishingIdle();
}

void DraftManager::publish(const DraftRecord &record)
{
    auto storage = NoteManager::instance()->storage(record.storageId);
    if (!storage || !storage->isAccessible()) {
        auto retry      = record;
        retry.state     = DraftRecord::Retry;
        retry.lastError = tr("Target storage is unavailable");
        retry.retryAt   = QDateTime::currentDateTimeUtc().addSecs(30);
        store_->write(retry);
        emit draftPublishFailed(record.id, retry.lastError);
        QTimer::singleShot(30000, this, &DraftManager::publishPending);
        return;
    }
    auto note = record.remoteNoteId.isEmpty() ? storage->createNote() : storage->note(record.remoteNoteId);
    if (note.isNull()) {
        emit draftPublishFailed(record.id, tr("Target note could not be created or loaded"));
        return;
    }
    note.setTitle(record.title);
    note.setText(record.body, record.format);
    auto publishing  = record;
    publishing.state = DraftRecord::Publishing;
    if (store_->write(publishing))
        return;
    publishing_.insert(record.id);
    auto *job = storage->saveNoteAsync(note, this);
    publishJobs_.insert(record.id, job);
    connect(job, &StorageJob::finished, this, [this, id = record.id, job]() {
        publishing_.remove(id);
        publishJobs_.remove(id);
        if (job->state() == StorageJob::Succeeded) {
            store_->remove(id);
            emit draftPublished(id, job->result());
        } else {
            auto retry = store_->load(id);
            if (retry) {
                retry.value.state     = DraftRecord::Retry;
                retry.value.lastError = job->error().message;
                retry.value.retryAt   = QDateTime::currentDateTimeUtc().addSecs(30);
                store_->write(retry.value);
            }
            emit draftPublishFailed(id, job->error().message);
            QTimer::singleShot(30000, this, &DraftManager::publishPending);
        }
        job->deleteLater();
        if (publishing_.isEmpty())
            emit publishingIdle();
    });
}

void DraftManager::storageAboutToBeRemoved(NoteStorage *storage)
{
    if (!store_ || !storage)
        return;
    auto records = store_->records();
    if (!records)
        return;
    int affected = 0;
    for (auto record : records.value) {
        if (record.storageId != storage->systemName())
            continue;
        if (auto job = publishJobs_.value(record.id))
            job->cancel();
        publishing_.remove(record.id);
        publishJobs_.remove(record.id);
        if (record.state != DraftRecord::Editing)
            record.state = DraftRecord::NeedsRouting;
        record.storageId.clear();
        record.remoteNoteId.clear();
        record.lastError = tr("The storage plugin was disabled; the remote original was left unchanged");
        record.retryAt   = {};
        if (!store_->write(record))
            ++affected;
    }
    if (affected) {
        emit publicationAbandoned(
            tr("%n note(s) could not be published because storage “%1” was disabled. The remote originals were "
               "left unchanged; local drafts will be routed as new notes.",
               nullptr, affected)
                .arg(storage->name()));
    }
}

} // namespace QtNote
