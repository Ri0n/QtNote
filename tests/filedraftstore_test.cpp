#include "conflictresolver.h"
#include "filedraftstore.h"
#include "secureenvelope.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace QtNote;

class FileDraftStoreTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void roundTrip();
    void deletionRoundTrip();
    void copyConflictResolution();
    void readsVersion2Draft();
    void rejectsWrongKey();
    void rejectsTampering();
};

void FileDraftStoreTest::initTestCase() { QVERIFY2(FileDraftStore::cryptoAvailable(), "AES-256-GCM unavailable"); }

static DraftRecord sampleRecord()
{
    DraftRecord record;
    record.id           = QUuid::createUuid();
    record.storageId    = QStringLiteral("nextcloud");
    record.remoteNoteId = QStringLiteral("remote-42");
    record.title        = QStringLiteral("Sensitive title");
    record.body         = QStringLiteral("Sensitive body that must not occur in the ciphertext");
    record.format       = Note::Markdown;
    record.tags         = { QStringLiteral("private"), QStringLiteral("work") };
    record.backendData.insert(QStringLiteral("etag"), QStringLiteral("base-etag"));
    record.backendData.insert(QStringLiteral("revision"), QStringLiteral("base-revision"));
    record.updatedAt = QDateTime::currentDateTimeUtc();
    return record;
}

static QString draftPath(const QString &root, const QUuid &id)
{
    return QDir(root).filePath(id.toString(QUuid::WithoutBraces) + QStringLiteral(".draft"));
}

void FileDraftStoreTest::roundTrip()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto key = FileDraftStore::generateMasterKey();
    QCOMPARE(key.size(), FileDraftStore::MasterKeySize);
    FileDraftStore store(directory.path(), key);
    auto           record = sampleRecord();
    QVERIFY(!store.write(record));

    QFile file(draftPath(directory.path(), record.id));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto encrypted = file.readAll();
    QVERIFY(!encrypted.contains(record.title.toUtf8()));
    QVERIFY(!encrypted.contains(record.body.toUtf8()));

    auto loaded = store.load(record.id);
    QVERIFY2(loaded, qPrintable(loaded.error.message));
    QCOMPARE(loaded.value.id, record.id);
    QCOMPARE(loaded.value.storageId, record.storageId);
    QCOMPARE(loaded.value.remoteNoteId, record.remoteNoteId);
    QCOMPARE(loaded.value.title, record.title);
    QCOMPARE(loaded.value.body, record.body);
    QCOMPARE(loaded.value.format, record.format);
    QCOMPARE(loaded.value.tags, record.tags);
    QCOMPARE(loaded.value.backendData, record.backendData);
    QCOMPARE(loaded.value.operation, DraftRecord::Publish);

    QVERIFY(!store.transition(record.id, DraftRecord::Ready));
    loaded = store.load(record.id);
    QVERIFY(loaded);
    QCOMPARE(loaded.value.state, DraftRecord::Ready);
}

void FileDraftStoreTest::copyConflictResolution()
{
    auto                 record = sampleRecord();
    CopyConflictResolver resolver;
    bool                 invoked = false;
    resolver.resolve({ record, {}, QStringLiteral("conflict") }, [&](ConflictResolution resolution) {
        invoked = true;
        QCOMPARE(resolution.action, ConflictResolution::CreateCopy);
        QVERIFY(resolution.copyTitle.startsWith(record.title + QStringLiteral(" (conflict ")));
        QVERIFY(resolution.notification.contains(resolution.copyTitle));
    });
    QVERIFY(invoked);
}

void FileDraftStoreTest::readsVersion2Draft()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto key    = FileDraftStore::generateMasterKey();
    const auto record = sampleRecord();

    QByteArray  legacy;
    QDataStream out(&legacy, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);
    out << quint32(0x514e4450) << quint16(2) << record.id << quint8(record.state) << record.storageId
        << record.remoteNoteId << record.title << record.body << quint8(record.format) << record.tags
        << record.updatedAt << record.lastError << record.retryAt << quint8(record.operation);
    QCOMPARE(out.status(), QDataStream::Ok);

    const AeadContext context { KeyDomain::LocalDraft, QStringLiteral("qtnote-local-drafts"),
                                record.id.toString(QUuid::WithoutBraces), 1, QStringLiteral("draft") };
    const auto        encrypted = SecureEnvelope::seal(legacy, key, context);
    QVERIFY2(encrypted, qPrintable(encrypted.error.message));
    QFile file(draftPath(directory.path(), record.id));
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(encrypted.value), encrypted.value.size());
    file.close();

    FileDraftStore store(directory.path(), key);
    const auto     loaded = store.load(record.id);
    QVERIFY2(loaded, qPrintable(loaded.error.message));
    QCOMPARE(loaded.value.title, record.title);
    QVERIFY(loaded.value.backendData.isEmpty());
}

void FileDraftStoreTest::deletionRoundTrip()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    FileDraftStore store(directory.path(), FileDraftStore::generateMasterKey());
    DraftRecord    record;
    record.id           = QUuid::createUuid();
    record.operation    = DraftRecord::Delete;
    record.state        = DraftRecord::Retry;
    record.storageId    = QStringLiteral("xmpp-pubsub");
    record.remoteNoteId = QStringLiteral("note-to-delete");
    QVERIFY(!store.write(record));

    const auto loaded = store.load(record.id);
    QVERIFY2(loaded, qPrintable(loaded.error.message));
    QCOMPARE(loaded.value.operation, DraftRecord::Delete);
    QCOMPARE(loaded.value.state, DraftRecord::Retry);
    QCOMPARE(loaded.value.storageId, record.storageId);
    QCOMPARE(loaded.value.remoteNoteId, record.remoteNoteId);
}

void FileDraftStoreTest::rejectsWrongKey()
{
    QTemporaryDir  directory;
    auto           record = sampleRecord();
    FileDraftStore writer(directory.path(), FileDraftStore::generateMasterKey());
    QVERIFY(!writer.write(record));
    FileDraftStore reader(directory.path(), FileDraftStore::generateMasterKey());
    auto           loaded = reader.load(record.id);
    QVERIFY(!loaded);
    QCOMPARE(loaded.error.code, DraftStoreError::Corrupt);
}

void FileDraftStoreTest::rejectsTampering()
{
    QTemporaryDir  directory;
    auto           record = sampleRecord();
    FileDraftStore store(directory.path(), FileDraftStore::generateMasterKey());
    QVERIFY(!store.write(record));
    QFile file(draftPath(directory.path(), record.id));
    QVERIFY(file.open(QIODevice::ReadWrite));
    auto bytes = file.readAll();
    QVERIFY(bytes.size() > 8);
    bytes[bytes.size() - 1] = char(bytes.back() ^ 0x01);
    QVERIFY(file.resize(0));
    QCOMPARE(file.write(bytes), bytes.size());
    file.close();
    auto loaded = store.load(record.id);
    QVERIFY(!loaded);
    QCOMPARE(loaded.error.code, DraftStoreError::Corrupt);
}

QTEST_GUILESS_MAIN(FileDraftStoreTest)
#include "filedraftstore_test.moc"
