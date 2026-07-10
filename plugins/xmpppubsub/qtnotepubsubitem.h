#ifndef QTNOTEPUBSUBITEM_H
#define QTNOTEPUBSUBITEM_H

#include "xmppdto.h"

#include <QXmppPubSubBaseItem.h>

namespace QtNote {

class QtNotePubSubItem final : public QXmppPubSubBaseItem {
public:
    static const QString payloadNamespace;

    QtNotePubSubItem() = default;
    explicit QtNotePubSubItem(const XmppRemoteNote &note);

    static bool isItem(const QDomElement &element);

    const XmppRemoteNote &note() const { return note_; }
    bool                  isValid() const { return valid_; }
    const QString        &parseError() const { return parseError_; }

protected:
    void parsePayload(const QDomElement &payloadElement) override;
    void serializePayload(QXmlStreamWriter *writer) const override;

private:
    XmppRemoteNote note_;
    bool           valid_ { false };
    QString        parseError_;
};

} // namespace QtNote

#endif // QTNOTEPUBSUBITEM_H
