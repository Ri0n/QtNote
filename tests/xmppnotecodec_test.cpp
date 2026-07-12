#include "xmppnotecodec.h"

#include <QtTest>

using namespace QtNote;

class XmppNoteCodecTest : public QObject {
    Q_OBJECT

private slots:
    void splitRoundTrip();
    void rejectsWrongNode();
    void rejectsMismatchedRevision();
};

static XmppRemoteNote note()
{
    XmppRemoteNote note;
    note.id       = QStringLiteral("note-id");
    note.revision = QStringLiteral("revision-1");
    note.originId = QStringLiteral("device");
    note.title    = QStringLiteral("Secret title");
    note.content  = QStringLiteral("Secret body");
    note.tags     = { QStringLiteral("one"), QStringLiteral("two") };
    note.modified = QDateTime::currentDateTimeUtc();
    return note;
}

void XmppNoteCodecTest::splitRoundTrip()
{
    const auto key         = SecureEnvelope::generateMasterKey();
    const auto source      = note();
    const auto indexNode   = QStringLiteral("urn:xmpp:qtnote:notes:index:1");
    const auto contentNode = QStringLiteral("urn:xmpp:qtnote:notes:content:1");
    auto       index       = XmppNoteCodec::encodeIndex(source, key, indexNode);
    auto       content     = XmppNoteCodec::encodeContent(source, key, contentNode);
    QVERIFY(index);
    QVERIFY(content);
    QVERIFY(!index.value.envelope.contains(source.title.toUtf8()));
    QVERIFY(!content.value.envelope.contains(source.content.toUtf8()));

    auto decodedIndex = XmppNoteCodec::decodeIndex(index.value, key, indexNode);
    QVERIFY(decodedIndex);
    QCOMPARE(decodedIndex.value.title, source.title);
    QCOMPARE(decodedIndex.value.tags, source.tags);
    QVERIFY(!decodedIndex.value.contentPresent);
    auto decodedContent = XmppNoteCodec::decodeContent(content.value, key, contentNode, decodedIndex.value);
    QVERIFY(decodedContent);
    QCOMPARE(decodedContent.value, source.content);
}

void XmppNoteCodecTest::rejectsWrongNode()
{
    const auto key     = SecureEnvelope::generateMasterKey();
    auto       payload = XmppNoteCodec::encodeIndex(note(), key, QStringLiteral("correct:index"));
    QVERIFY(payload);
    QVERIFY(!XmppNoteCodec::decodeIndex(payload.value, key, QStringLiteral("other:index")));
}

void XmppNoteCodecTest::rejectsMismatchedRevision()
{
    const auto key     = SecureEnvelope::generateMasterKey();
    auto       source  = note();
    auto       payload = XmppNoteCodec::encodeContent(source, key, QStringLiteral("content"));
    QVERIFY(payload);
    source.revision = QStringLiteral("revision-2");
    QVERIFY(!XmppNoteCodec::decodeContent(payload.value, key, QStringLiteral("content"), source));
}

QTEST_GUILESS_MAIN(XmppNoteCodecTest)
#include "xmppnotecodec_test.moc"
