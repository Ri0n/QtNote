#include "xmppomemopubsubitems.h"

#include <QDomElement>
#include <QXmlStreamWriter>

namespace QtNote {
namespace {
    const QString OmemoNamespace = QStringLiteral("urn:xmpp:omemo:2");

    QString localName(const QDomElement &element)
    {
        return element.localName().isEmpty() ? element.tagName().section(QLatin1Char(':'), -1) : element.localName();
    }

    bool hasPayload(const QDomElement &item, const QString &name)
    {
        if (!QXmppPubSubBaseItem::isItem(item))
            return false;
        const auto payload = item.firstChildElement();
        return !payload.isNull() && localName(payload) == name && payload.namespaceURI() == OmemoNamespace;
    }
}

bool XmppOmemoDeviceListItem::isItem(const QDomElement &element)
{
    return hasPayload(element, QStringLiteral("devices"));
}

void XmppOmemoDeviceListItem::parsePayload(const QDomElement &element)
{
    devices_.clear();
    for (auto device = element.firstChildElement(); !device.isNull(); device = device.nextSiblingElement()) {
        if (localName(device) != QStringLiteral("device") || device.namespaceURI() != OmemoNamespace)
            continue;
        const auto id   = device.attribute(QStringLiteral("id"));
        bool       idOk = false;
        id.toUInt(&idOk);
        if (idOk)
            devices_.append(
                { device.attribute(QStringLiteral("label")), id, device.attribute(QStringLiteral("labelsig")) });
    }
}

void XmppOmemoDeviceListItem::serializePayload(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("devices"));
    writer->writeDefaultNamespace(OmemoNamespace);
    for (const auto &device : devices_) {
        writer->writeStartElement(QStringLiteral("device"));
        writer->writeAttribute(QStringLiteral("id"), device.id);
        if (!device.label.isEmpty())
            writer->writeAttribute(QStringLiteral("label"), device.label);
        if (!device.labelSignature.isEmpty())
            writer->writeAttribute(QStringLiteral("labelsig"), device.labelSignature);
        writer->writeEndElement();
    }
    writer->writeEndElement();
}

bool XmppOmemoBundleItem::isItem(const QDomElement &element) { return hasPayload(element, QStringLiteral("bundle")); }

void XmppOmemoBundleItem::parsePayload(const QDomElement &element)
{
    identityKey_.clear();
    for (auto child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
        if (localName(child) == QStringLiteral("ik") && child.namespaceURI() == OmemoNamespace) {
            identityKey_ = QByteArray::fromBase64(child.text().trimmed().toLatin1());
            return;
        }
    }
}

} // namespace QtNote
