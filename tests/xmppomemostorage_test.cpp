#include "xmppomemostorage.h"

#include "secureenvelope.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace QtNote;

class XmppOmemoStorageTest : public QObject {
    Q_OBJECT

private slots:
    void persistsEncryptedState();
    void rejectsWrongLocalKey();
};

void XmppOmemoStorageTest::persistsEncryptedState()
{
    QTemporaryDir dir;
    const auto    path = dir.filePath(QStringLiteral("omemo.state"));
    const auto    key  = SecureEnvelope::generateMasterKey();
    {
        XmppOmemoStorage storage(path, key, QStringLiteral("me@example.org"));
        QVERIFY(storage.isValid());
        QXmppOmemoStorage::OwnDevice own;
        own.id                 = 42;
        own.label              = QStringLiteral("Secret device label");
        own.privateIdentityKey = QByteArrayLiteral("private identity material");
        own.publicIdentityKey  = QByteArrayLiteral("public identity material");
        storage.setOwnDevice(own);
        QXmppOmemoStorage::Device peer;
        peer.label   = QStringLiteral("Other device");
        peer.keyId   = QByteArrayLiteral("peer-key-id");
        peer.session = QByteArrayLiteral("session-secret");
        storage.addDevice(QStringLiteral("me@example.org"), 7, peer);
        QVERIFY(storage.isValid());
    }
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto encrypted = file.readAll();
    QVERIFY(!encrypted.contains("private identity material"));
    QVERIFY(!encrypted.contains("session-secret"));

    XmppOmemoStorage restored(path, key, QStringLiteral("me@example.org"));
    QVERIFY2(restored.isValid(), qPrintable(restored.errorString()));
    auto task = restored.allData();
    QVERIFY(task.isFinished());
    auto data = task.takeResult();
    QVERIFY(data.ownDevice.has_value());
    QCOMPARE(data.ownDevice->id, uint32_t(42));
    QCOMPARE(data.devices[QStringLiteral("me@example.org")][7].session, QByteArrayLiteral("session-secret"));
}

void XmppOmemoStorageTest::rejectsWrongLocalKey()
{
    QTemporaryDir dir;
    const auto    path = dir.filePath(QStringLiteral("omemo.state"));
    {
        XmppOmemoStorage storage(path, SecureEnvelope::generateMasterKey(), QStringLiteral("me@example.org"));
        QXmppOmemoStorage::OwnDevice own;
        own.id = 1;
        storage.setOwnDevice(own);
    }
    XmppOmemoStorage wrong(path, SecureEnvelope::generateMasterKey(), QStringLiteral("me@example.org"));
    QVERIFY(!wrong.isValid());
}

QTEST_GUILESS_MAIN(XmppOmemoStorageTest)
#include "xmppomemostorage_test.moc"
