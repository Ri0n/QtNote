#include "xmppnotecodec.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace QtNote {
namespace {
    CryptoError invalid(const QString &message) { return { CryptoError::Corrupt, message }; }

    AeadContext context(const XmppEncryptedPayload &payload, const QString &nodeName)
    {
        const bool index = payload.kind == XmppEncryptedPayload::Index;
        return { index ? KeyDomain::StorageIndex : KeyDomain::StorageContent, nodeName, payload.id, payload.schema,
                 index ? QStringLiteral("index") : QStringLiteral("content") };
    }

    CryptoResult<XmppEncryptedPayload> encode(const QJsonObject &json, const XmppRemoteNote &note,
                                              XmppEncryptedPayload::Kind kind, const QByteArray &masterKey,
                                              const QString &nodeName)
    {
        XmppEncryptedPayload payload;
        payload.id    = note.id;
        payload.kind  = kind;
        payload.keyId = SecureEnvelope::keyId(masterKey);
        if (payload.id.isEmpty() || payload.keyId.isEmpty())
            return { {}, { CryptoError::InvalidArgument, QStringLiteral("Missing note id or storage key") } };
        auto sealed = SecureEnvelope::seal(QJsonDocument(json).toJson(QJsonDocument::Compact), masterKey,
                                           context(payload, nodeName));
        if (!sealed)
            return { {}, sealed.error };
        payload.envelope = sealed.value;
        return { payload, {} };
    }

    CryptoResult<QJsonObject> decode(const XmppEncryptedPayload &payload, XmppEncryptedPayload::Kind expected,
                                     const QByteArray &masterKey, const QString &nodeName)
    {
        if (payload.kind != expected || payload.keyId != SecureEnvelope::keyId(masterKey))
            return { {}, invalid(QStringLiteral("Encrypted QtNote key or payload kind mismatch")) };
        auto opened = SecureEnvelope::open(payload.envelope, masterKey, context(payload, nodeName));
        if (!opened)
            return { {}, opened.error };
        QJsonParseError parseError;
        const auto      document = QJsonDocument::fromJson(opened.value, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
            return { {}, invalid(QStringLiteral("Invalid encrypted QtNote JSON")) };
        return { document.object(), {} };
    }
}

CryptoResult<XmppEncryptedPayload> XmppNoteCodec::encodeIndex(const XmppRemoteNote &note, const QByteArray &masterKey,
                                                              const QString &nodeName)
{
    QJsonObject json { { QStringLiteral("id"), note.id },
                       { QStringLiteral("revision"), note.revision },
                       { QStringLiteral("parentRevision"), note.parentRevision },
                       { QStringLiteral("originId"), note.originId },
                       { QStringLiteral("title"), note.title },
                       { QStringLiteral("modified"), note.modified.toUTC().toString(Qt::ISODateWithMs) },
                       { QStringLiteral("format"), note.format },
                       { QStringLiteral("tags"), QJsonArray::fromStringList(note.tags) } };
    return encode(json, note, XmppEncryptedPayload::Index, masterKey, nodeName);
}

CryptoResult<XmppEncryptedPayload> XmppNoteCodec::encodeContent(const XmppRemoteNote &note, const QByteArray &masterKey,
                                                                const QString &nodeName)
{
    const QJsonObject json { { QStringLiteral("id"), note.id },
                             { QStringLiteral("revision"), note.revision },
                             { QStringLiteral("content"), note.content } };
    return encode(json, note, XmppEncryptedPayload::Content, masterKey, nodeName);
}

CryptoResult<XmppRemoteNote> XmppNoteCodec::decodeIndex(const XmppEncryptedPayload &payload,
                                                        const QByteArray &masterKey, const QString &nodeName)
{
    auto decoded = decode(payload, XmppEncryptedPayload::Index, masterKey, nodeName);
    if (!decoded)
        return { {}, decoded.error };
    const auto    &json = decoded.value;
    XmppRemoteNote note;
    note.id             = json.value(QStringLiteral("id")).toString();
    note.revision       = json.value(QStringLiteral("revision")).toString();
    note.parentRevision = json.value(QStringLiteral("parentRevision")).toString();
    note.originId       = json.value(QStringLiteral("originId")).toString();
    note.title          = json.value(QStringLiteral("title")).toString();
    note.modified       = QDateTime::fromString(json.value(QStringLiteral("modified")).toString(), Qt::ISODateWithMs);
    note.format         = json.value(QStringLiteral("format")).toString();
    for (const auto &tag : json.value(QStringLiteral("tags")).toArray())
        note.tags.append(tag.toString());
    note.contentPresent = false;
    if (note.id != payload.id || note.revision.isEmpty() || !note.modified.isValid()
        || note.format != QStringLiteral("markdown")) {
        return { {}, invalid(QStringLiteral("Invalid encrypted QtNote index")) };
    }
    note.modified = note.modified.toUTC();
    return { note, {} };
}

CryptoResult<QString> XmppNoteCodec::decodeContent(const XmppEncryptedPayload &payload, const QByteArray &masterKey,
                                                   const QString &nodeName, const XmppRemoteNote &index)
{
    auto decoded = decode(payload, XmppEncryptedPayload::Content, masterKey, nodeName);
    if (!decoded)
        return { {}, decoded.error };
    const auto &json = decoded.value;
    if (json.value(QStringLiteral("id")).toString() != payload.id || payload.id != index.id
        || json.value(QStringLiteral("revision")).toString() != index.revision) {
        return { {}, invalid(QStringLiteral("QtNote content does not match its index revision")) };
    }
    return { json.value(QStringLiteral("content")).toString(), {} };
}

} // namespace QtNote
