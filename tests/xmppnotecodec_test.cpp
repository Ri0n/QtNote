#include "xmppbackend.h"
#include "xmppnotecodec.h"

#include <QtTest>

#include <type_traits>

using namespace QtNote;

// Async backend inputs must be owned values. This guards against coroutine
// implementations retaining references to caller-owned QString/QByteArray
// objects across suspension.
static_assert(
    std::is_same_v<decltype(&XmppBackend::getNoteAsync), void (XmppBackend::*)(QString, XmppBackend::NoteCallback)>);
static_assert(std::is_same_v<decltype(&XmppBackend::saveNoteAsync),
                             void (XmppBackend::*)(XmppRemoteNote, XmppBackend::NoteCallback)>);
static_assert(std::is_same_v<decltype(&XmppBackend::deleteNoteAsync),
                             void (XmppBackend::*)(QString, XmppBackend::StatusCallback)>);

class XmppNoteCodecTest : public QObject {
    Q_OBJECT

private slots:
    void splitRoundTrip();
    void rejectsWrongNode();
    void reportsWrongStorageKey();
    void rejectsMismatchedRevision();
    void rekeysMixedIndexAndContentKeys();
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

void XmppNoteCodecTest::reportsWrongStorageKey()
{
    const auto key     = SecureEnvelope::generateMasterKey();
    auto       payload = XmppNoteCodec::encodeIndex(note(), key, QStringLiteral("index"));
    QVERIFY(payload);
    const auto decoded
        = XmppNoteCodec::decodeIndex(payload.value, SecureEnvelope::generateMasterKey(), QStringLiteral("index"));
    QVERIFY(!decoded);
    QVERIFY(decoded.error.message.contains(QStringLiteral("storage key mismatch")));
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

void XmppNoteCodecTest::rekeysMixedIndexAndContentKeys()
{
    const auto indexKey     = SecureEnvelope::generateMasterKey();
    const auto contentKey   = SecureEnvelope::generateMasterKey();
    const auto canonicalKey = SecureEnvelope::generateMasterKey();
    const auto source       = note();
    const auto indexNode    = QStringLiteral("index");
    const auto contentNode  = QStringLiteral("content");
    auto       oldIndex     = XmppNoteCodec::encodeIndex(source, indexKey, indexNode);
    auto       oldContent   = XmppNoteCodec::encodeContent(source, contentKey, contentNode);
    QVERIFY(oldIndex);
    QVERIFY(oldContent);
    QVERIFY(oldIndex.value.keyId != oldContent.value.keyId);

    auto decodedIndex = XmppNoteCodec::decodeIndex(oldIndex.value, indexKey, indexNode);
    QVERIFY(decodedIndex);
    auto decodedContent = XmppNoteCodec::decodeContent(oldContent.value, contentKey, contentNode, decodedIndex.value);
    QVERIFY(decodedContent);
    decodedIndex.value.content        = decodedContent.value;
    decodedIndex.value.contentPresent = true;

    auto newContent = XmppNoteCodec::encodeContent(decodedIndex.value, canonicalKey, contentNode);
    auto newIndex   = XmppNoteCodec::encodeIndex(decodedIndex.value, canonicalKey, indexNode);
    QVERIFY(newContent);
    QVERIFY(newIndex);
    QCOMPARE(newContent.value.keyId, newIndex.value.keyId);
    QCOMPARE(newIndex.value.keyId, SecureEnvelope::keyId(canonicalKey));
    auto canonicalIndex = XmppNoteCodec::decodeIndex(newIndex.value, canonicalKey, indexNode);
    QVERIFY(canonicalIndex);
    auto canonicalContent
        = XmppNoteCodec::decodeContent(newContent.value, canonicalKey, contentNode, canonicalIndex.value);
    QVERIFY(canonicalContent);
    QCOMPARE(canonicalContent.value, source.content);
}

QTEST_GUILESS_MAIN(XmppNoteCodecTest)
#include "xmppnotecodec_test.moc"
