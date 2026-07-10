#include "xmpppepextension.h"

#include "qtnotepubsubitem.h"

#include <QDomElement>
#include <QXmppPubSubEvent.h>
#include <QXmppUtils.h>

namespace QtNote {

XmppPepExtension::XmppPepExtension() = default;

void XmppPepExtension::setOwnBareJid(const QString &jid) { ownBareJid_ = QXmppUtils::jidToBareJid(jid); }

void XmppPepExtension::setNodeName(const QString &nodeName) { nodeName_ = nodeName; }

QStringList XmppPepExtension::discoveryFeatures() const
{
    if (nodeName_.isEmpty()) {
        return {};
    }
    return { nodeName_ + QStringLiteral("+notify") };
}

bool XmppPepExtension::handlePubSubEvent(const QDomElement &element, const QString &pubSubService,
                                         const QString &nodeName)
{
    if (nodeName != nodeName_) {
        return false;
    }

    // XEP-0223 requires private-data events to originate from our own PEP
    // service. Some servers omit the service JID, which is valid here.
    if (!pubSubService.isEmpty() && QXmppUtils::jidToBareJid(pubSubService) != ownBareJid_) {
        return true;
    }

    if (!QXmppPubSubEvent<QtNotePubSubItem>::isPubSubEvent(element)) {
        emit malformedItem(QStringLiteral("Malformed QtNote PubSub event"));
        return true;
    }

    QXmppPubSubEvent<QtNotePubSubItem> event;
    event.parse(element);

    switch (event.eventType()) {
    case QXmppPubSubEventBase::Items:
        for (const auto &item : event.items()) {
            if (item.isValid()) {
                emit notePublished(item.note());
            } else {
                emit malformedItem(item.parseError());
            }
        }
        break;
    case QXmppPubSubEventBase::Retract:
        for (const auto &id : event.retractIds()) {
            emit noteRetracted(id);
        }
        break;
    case QXmppPubSubEventBase::Delete:
    case QXmppPubSubEventBase::Purge:
        emit nodeInvalidated();
        break;
    case QXmppPubSubEventBase::Configuration:
    case QXmppPubSubEventBase::Subscription:
        break;
    }

    return true;
}

} // namespace QtNote
