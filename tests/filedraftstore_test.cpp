#include "filedraftstore.h"

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
    record.updatedAt    = QDateTime::currentDateTimeUtc();
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
    QCOMPARE(loaded.value.operation, DraftRecord::Publish);

    QVERIFY(!store.transition(record.id, DraftRecord::Ready));
    loaded = store.load(record.id);
    QVERIFY(loaded);
    QCOMPARE(loaded.value.state, DraftRecord::Ready);
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
