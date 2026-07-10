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

    QDomElement firstChild(const QDomElement &parent, const QString &localName)
    {
        for (auto child = parent.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
            if (elementLocalName(child) == localName && child.namespaceURI() == QtNotePubSubItem::payloadNamespace) {
                return child;
            }
        }
        return {};
    }

} // namespace

const QString QtNotePubSubItem::payloadNamespace = QStringLiteral("urn:xmpp:qtnote:note:0");

QtNotePubSubItem::QtNotePubSubItem(const XmppRemoteNote &note) : QXmppPubSubBaseItem(note.id), note_(note), valid_(true)
{
}

bool QtNotePubSubItem::isItem(const QDomElement &element)
{
    if (!QXmppPubSubBaseItem::isItem(element)) {
        return false;
    }

    const auto payload = element.firstChildElement();
    return !payload.isNull() && elementLocalName(payload) == QStringLiteral("note")
        && payload.namespaceURI() == payloadNamespace;
}

void QtNotePubSubItem::parsePayload(const QDomElement &payloadElement)
{
    valid_ = true;
    parseError_.clear();
    note_    = {};
    note_.id = id();

    const QString schema = payloadElement.attribute(QStringLiteral("schema"));
    if (schema != QStringLiteral("1")) {
        valid_      = false;
        parseError_ = QStringLiteral("Unsupported QtNote payload schema: %1").arg(schema);
        return;
    }
    note_.revision       = payloadElement.attribute(QStringLiteral("revision"));
    note_.parentRevision = payloadElement.attribute(QStringLiteral("parent-revision"));
    note_.originId       = payloadElement.attribute(QStringLiteral("origin"));
    note_.format         = payloadElement.attribute(QStringLiteral("format"), QStringLiteral("markdown"));

    const QString modifiedText = payloadElement.attribute(QStringLiteral("modified"));
    note_.modified             = QDateTime::fromString(modifiedText, Qt::ISODateWithMs);
    if (!note_.modified.isValid()) {
        note_.modified = QDateTime::fromString(modifiedText, Qt::ISODate);
    }
    if (!note_.modified.isValid()) {
        valid_      = false;
        parseError_ = QStringLiteral("QtNote item has an invalid modified timestamp");
        return;
    }
    note_.modified = note_.modified.toUTC();

    if (note_.format != QStringLiteral("markdown")) {
        valid_      = false;
        parseError_ = QStringLiteral("Unsupported QtNote note format: %1").arg(note_.format);
        return;
    }

    const auto plaintext = firstChild(payloadElement, QStringLiteral("plaintext"));
    if (plaintext.isNull() || plaintext.namespaceURI() != payloadNamespace) {
        valid_      = false;
        parseError_ = QStringLiteral("Unsupported QtNote payload: expected an unencrypted <plaintext> element");
        note_.contentPresent = false;
        return;
    }

    const auto title   = firstChild(plaintext, QStringLiteral("title"));
    const auto content = firstChild(plaintext, QStringLiteral("content"));
    if (title.isNull() || content.isNull() || title.namespaceURI() != payloadNamespace
        || content.namespaceURI() != payloadNamespace) {
        valid_               = false;
        parseError_          = QStringLiteral("QtNote plaintext payload has no title or content element");
        note_.contentPresent = false;
        return;
    }

    note_.title          = title.text();
    note_.content        = content.text();
    note_.contentPresent = true;

    if (note_.id.isEmpty() || note_.revision.isEmpty()) {
        valid_      = false;
        parseError_ = QStringLiteral("QtNote item has no id or revision");
    }
}

void QtNotePubSubItem::serializePayload(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("note"));
    writer->writeDefaultNamespace(payloadNamespace);
    writer->writeAttribute(QStringLiteral("schema"), QStringLiteral("1"));
    writer->writeAttribute(QStringLiteral("revision"), note_.revision);
    if (!note_.parentRevision.isEmpty()) {
        writer->writeAttribute(QStringLiteral("parent-revision"), note_.parentRevision);
    }
    if (!note_.originId.isEmpty()) {
        writer->writeAttribute(QStringLiteral("origin"), note_.originId);
    }
    writer->writeAttribute(QStringLiteral("modified"), note_.modified.toUTC().toString(Qt::ISODateWithMs));
    writer->writeAttribute(QStringLiteral("format"), note_.format);

    writer->writeStartElement(QStringLiteral("plaintext"));
    writer->writeTextElement(QStringLiteral("title"), note_.title);
    writer->writeTextElement(QStringLiteral("content"), note_.content);
    writer->writeEndElement(); // plaintext
    writer->writeEndElement(); // note
}

} // namespace QtNote
