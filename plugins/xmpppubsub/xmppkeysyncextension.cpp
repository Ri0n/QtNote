#include "xmppkeysyncextension.h"

#include <QDomElement>
#include <QJsonDocument>
#include <QJsonObject>
#include <QXmppClient.h>
#include <QXmppElement.h>
#include <QXmppTask.h>

namespace QtNote {
namespace {
    QDomElement keySyncElement(const QDomElement &iq)
    {
        const auto child = iq.firstChildElement();
        const auto name  = child.localName().isEmpty() ? child.tagName() : child.localName();
        const auto ns
            = child.namespaceURI().isEmpty() ? child.attribute(QStringLiteral("xmlns")) : child.namespaceURI();
        return ns == XmppKeySyncExtension::feature && name == QStringLiteral("key-sync") ? child : QDomElement {};
    }

    QXmppElement payloadElement(const QJsonObject &payload)
    {
        QXmppElement element;
        element.setTagName(QStringLiteral("key-sync"));
        element.setAttribute(QStringLiteral("xmlns"), XmppKeySyncExtension::feature);
        element.setValue(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact)));
        return element;
    }

    QJsonObject parsePayload(const QDomElement &element)
    {
        QJsonParseError error;
        const auto      document = QJsonDocument::fromJson(element.text().toUtf8(), &error);
        return error.error == QJsonParseError::NoError && document.isObject() ? document.object() : QJsonObject {};
    }
}

const QString XmppKeySyncExtension::feature = QStringLiteral("urn:xmpp:qtnote:key-sync:1");

QStringList XmppKeySyncExtension::discoveryFeatures() const { return { feature }; }

bool XmppKeySyncExtension::handleStanza(const QDomElement &stanza, const std::optional<QXmppE2eeMetadata> &metadata)
{
    if (stanza.tagName() != QStringLiteral("iq") || stanza.attribute(QStringLiteral("type")) != QStringLiteral("set"))
        return false;
    const auto element = keySyncElement(stanza);
    if (element.isNull())
        return false;
    const auto payload   = parsePayload(element);
    const auto requestId = payload.value(QStringLiteral("requestId")).toString();
    if (payload.value(QStringLiteral("type")).toString() != QStringLiteral("request") || requestId.isEmpty()
        || !metadata) {
        return true;
    }
    pendingRequests_.insert(
        requestId, { stanza.attribute(QStringLiteral("from")), stanza.attribute(QStringLiteral("id")), metadata });
    emit requestReceived(requestId, stanza.attribute(QStringLiteral("from")), metadata->senderKey());
    return true;
}

QXmppIq XmppKeySyncExtension::makeRequest(const QString &to, const QString &requestId) const
{
    QXmppIq iq(QXmppIq::Set);
    iq.setTo(to);
    iq.setId(requestId);
    iq.setExtensions({ payloadElement(
        { { QStringLiteral("type"), QStringLiteral("request") }, { QStringLiteral("requestId"), requestId } }) });
    return iq;
}

void XmppKeySyncExtension::replyWithKey(const QString &requestId, const QString &recoveryKey)
{
    const auto pending = pendingRequests_.take(requestId);
    if (pending.iqId.isEmpty())
        return;
    QXmppIq response(QXmppIq::Result);
    response.setTo(pending.from);
    response.setId(pending.iqId);
    response.setExtensions({ payloadElement({ { QStringLiteral("type"), QStringLiteral("response") },
                                              { QStringLiteral("requestId"), requestId },
                                              { QStringLiteral("recoveryKey"), recoveryKey } }) });
    client()->reply(std::move(response), pending.metadata);
}

void XmppKeySyncExtension::reject(const QString &requestId) { pendingRequests_.remove(requestId); }

QString XmppKeySyncExtension::responseRecoveryKey(const QDomElement &iq, const QString &requestId)
{
    const auto payload = parsePayload(keySyncElement(iq));
    if (payload.value(QStringLiteral("type")).toString() != QStringLiteral("response")
        || payload.value(QStringLiteral("requestId")).toString() != requestId) {
        return {};
    }
    return payload.value(QStringLiteral("recoveryKey")).toString();
}

} // namespace QtNote
