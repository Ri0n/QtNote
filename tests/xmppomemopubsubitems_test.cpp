#include "xmppomemopubsubitems.h"

#include <QDomDocument>
#include <QtTest>

using namespace QtNote;

class XmppOmemoPubSubItemsTest : public QObject {
    Q_OBJECT

private slots:
    void parsesDeviceList();
    void parsesBundleIdentityKey();
};

void XmppOmemoPubSubItemsTest::parsesDeviceList()
{
    QDomDocument document;
    QVERIFY(
        document.setContent(QStringLiteral("<item id='current'><devices xmlns='urn:xmpp:omemo:2'>"
                                           "<device id='9523' label='QtNote-one'/><device id='672' label='QtNote-two'/>"
                                           "</devices></item>"),
                            QDomDocument::ParseOption::UseNamespaceProcessing));
    XmppOmemoDeviceListItem item;
    item.parse(document.documentElement());
    QCOMPARE(item.devices().size(), 2);
    QCOMPARE(item.devices().at(1).id, QStringLiteral("672"));
    QCOMPARE(item.devices().at(1).label, QStringLiteral("QtNote-two"));
}

void XmppOmemoPubSubItemsTest::parsesBundleIdentityKey()
{
    const auto   identity = QByteArray::fromHex("00112233445566778899aabbccddeeff");
    QDomDocument document;
    QVERIFY(document.setContent(
        QStringLiteral("<item id='672'><bundle xmlns='urn:xmpp:omemo:2'><ik>%1</ik></bundle></item>")
            .arg(QString::fromLatin1(identity.toBase64())),
        QDomDocument::ParseOption::UseNamespaceProcessing));
    XmppOmemoBundleItem item;
    item.parse(document.documentElement());
    QCOMPARE(item.identityKey(), identity);
}

QTEST_GUILESS_MAIN(XmppOmemoPubSubItemsTest)
#include "xmppomemopubsubitems_test.moc"
