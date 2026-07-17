#include "fileremotecachestore.h"
#include "secureenvelope.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace QtNote;

class FileRemoteCacheStoreTest : public QObject {
    Q_OBJECT

private slots:
    void roundTrip();
    void rejectsWrongInstance();
    void rejectsTampering();
};

static RemoteCacheRecord sampleRecord()
{
    RemoteCacheRecord record;
    record.id          = QStringLiteral("note-1");
    record.title       = QStringLiteral("Cached note");
    record.tags        = { QStringLiteral("offline") };
    record.modified    = QDateTime::fromSecsSinceEpoch(1700000000, Qt::UTC);
    record.format      = Note::Markdown;
    record.body        = QStringLiteral("# Cached note\nBody");
    record.bodyPresent = true;
    record.backendData.insert(QStringLiteral("revision"), QStringLiteral("r1"));
    record.syncState    = RemoteCacheRecord::Synced;
    record.lastOpenedAt = QDateTime::fromSecsSinceEpoch(1700000100, Qt::UTC);
    return record;
}

void FileRemoteCacheStoreTest::roundTrip()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto           key = SecureEnvelope::generateMasterKey();
    FileRemoteCacheStore store(directory.filePath(QStringLiteral("cache.bin")), QStringLiteral("instance-1"), key);
    QVERIFY(!store.replaceRecords({ sampleRecord() }));

    const auto loaded = store.records();
    QVERIFY(loaded);
    QCOMPARE(loaded.value.size(), 1);
    const auto &record = loaded.value.first();
    QCOMPARE(record.id, QStringLiteral("note-1"));
    QCOMPARE(record.body, QStringLiteral("# Cached note\nBody"));
    QVERIFY(record.bodyPresent);
    QCOMPARE(record.backendData.value(QStringLiteral("revision")).toString(), QStringLiteral("r1"));
    QVERIFY(record.cachedAt.isValid());

    auto changed = record;
    changed.body = QStringLiteral("changed");
    QVERIFY(!store.put(changed));
    QCOMPARE(store.records().value.first().body, QStringLiteral("changed"));
    QVERIFY(!store.remove(record.id));
    QVERIFY(store.records().value.isEmpty());
}

void FileRemoteCacheStoreTest::rejectsWrongInstance()
{
    QTemporaryDir        directory;
    const auto           path = directory.filePath(QStringLiteral("cache.bin"));
    const auto           key  = SecureEnvelope::generateMasterKey();
    FileRemoteCacheStore writer(path, QStringLiteral("instance-1"), key);
    QVERIFY(!writer.put(sampleRecord()));
    FileRemoteCacheStore reader(path, QStringLiteral("instance-2"), key);
    QCOMPARE(reader.records().error.code, RemoteCacheError::Corrupt);
}

void FileRemoteCacheStoreTest::rejectsTampering()
{
    QTemporaryDir        directory;
    const auto           path = directory.filePath(QStringLiteral("cache.bin"));
    FileRemoteCacheStore store(path, QStringLiteral("instance-1"), SecureEnvelope::generateMasterKey());
    QVERIFY(!store.put(sampleRecord()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadWrite));
    auto bytes              = file.readAll();
    bytes[bytes.size() / 2] = char(uchar(bytes.at(bytes.size() / 2)) ^ 1U);
    file.resize(0);
    QCOMPARE(file.write(bytes), bytes.size());
    file.close();
    QCOMPARE(store.records().error.code, RemoteCacheError::Corrupt);
}

QTEST_GUILESS_MAIN(FileRemoteCacheStoreTest)
#include "fileremotecachestore_test.moc"
