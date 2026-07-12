#include "qtnotepubsubitem.h"

#include <QDomElement>
#include <QXmlStreamWriter>

namespace QtNote {
namespace {
    QString elementLocalName(const QDomElement &element)
    {
        const auto local = element.localName();
        return local.isEmpty() ? element.tagName().section(QLatin1Char(':'), -1) : local;
    }
}

const QString QtNotePubSubItem::payloadNamespace = QStringLiteral("urn:xmpp:qtnote:encrypted:1");

QtNotePubSubItem::QtNotePubSubItem(const XmppEncryptedPayload &payload) :
    QXmppPubSubBaseItem(payload.id), payload_(payload), valid_(true)
{
}

bool QtNotePubSubItem::isItem(const QDomElement &element)
{
    if (!QXmppPubSubBaseItem::isItem(element))
        return false;
    const auto payload = element.firstChildElement();
    return !payload.isNull() && elementLocalName(payload) == QStringLiteral("encrypted")
        && payload.namespaceURI() == payloadNamespace;
}

void QtNotePubSubItem::parsePayload(const QDomElement &element)
{
    valid_ = false;
    parseError_.clear();
    payload_    = {};
    payload_.id = id();

    bool schemaOk   = false;
    payload_.schema = element.attribute(QStringLiteral("schema")).toUInt(&schemaOk);
    if (!schemaOk || payload_.schema != 1) {
        parseError_ = QStringLiteral("Unsupported encrypted QtNote schema");
        return;
    }
    const auto kind = element.attribute(QStringLiteral("kind"));
    if (kind == QStringLiteral("index"))
        payload_.kind = XmppEncryptedPayload::Index;
    else if (kind == QStringLiteral("content"))
        payload_.kind = XmppEncryptedPayload::Content;
    else {
        parseError_ = QStringLiteral("Unknown encrypted QtNote payload kind");
        return;
    }
    payload_.keyId
        = QByteArray::fromBase64(element.attribute(QStringLiteral("key-id")).toLatin1(), QByteArray::Base64UrlEncoding);
    payload_.envelope = QByteArray::fromBase64(element.text().trimmed().toLatin1(), QByteArray::Base64Encoding);
    if (payload_.id.isEmpty() || payload_.keyId.size() != 32 || payload_.envelope.isEmpty()) {
        parseError_ = QStringLiteral("Incomplete encrypted QtNote payload");
        return;
    }
    valid_ = true;
}

void QtNotePubSubItem::serializePayload(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("encrypted"));
    writer->writeDefaultNamespace(payloadNamespace);
    writer->writeAttribute(QStringLiteral("schema"), QString::number(payload_.schema));
    writer->writeAttribute(QStringLiteral("kind"),
                           payload_.kind == XmppEncryptedPayload::Index ? QStringLiteral("index")
                                                                        : QStringLiteral("content"));
    writer->writeAttribute(
        QStringLiteral("key-id"),
        QString::fromLatin1(payload_.keyId.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
    writer->writeCharacters(QString::fromLatin1(payload_.envelope.toBase64()));
    writer->writeEndElement();
}

} // namespace QtNote
