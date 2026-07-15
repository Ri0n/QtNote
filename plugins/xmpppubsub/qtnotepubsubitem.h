#ifndef QTNOTEPUBSUBITEM_H
#define QTNOTEPUBSUBITEM_H

#include "xmppdto.h"

#include <QXmppPubSubBaseItem.h>

namespace QtNote {

/**
 * @brief QXmpp PubSub item adapter for QtNote encrypted payloads.
 *
 * The class validates the application namespace and transports the opaque
 * encrypted envelope. Plain note metadata is never exposed in the PubSub XML.
 */
class QtNotePubSubItem final : public QXmppPubSubBaseItem {
public:
    static const QString payloadNamespace;

    QtNotePubSubItem() = default;
    explicit QtNotePubSubItem(const XmppEncryptedPayload &payload);

    /// Returns whether @p element is a QtNote encrypted PubSub item.
    static bool isItem(const QDomElement &element);

    const XmppEncryptedPayload &payload() const { return payload_; }
    bool                        isValid() const { return valid_; }
    const QString              &parseError() const { return parseError_; }

protected:
    void parsePayload(const QDomElement &payloadElement) override;
    void serializePayload(QXmlStreamWriter *writer) const override;

private:
    XmppEncryptedPayload payload_;
    /// True only after all mandatory XML attributes and envelope data parsed.
    bool valid_ { false };
    /// Human-readable validation failure when isValid() is false.
    QString parseError_;
};

} // namespace QtNote

#endif // QTNOTEPUBSUBITEM_H
