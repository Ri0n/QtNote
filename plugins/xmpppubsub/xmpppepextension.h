#ifndef XMPPPEPEXTENSION_H
#define XMPPPEPEXTENSION_H

#include "xmppdto.h"

#include <QXmppClientExtension.h>
#include <QXmppPubSubEventHandler.h>

namespace QtNote {

class XmppPepExtension final : public QXmppClientExtension, public QXmppPubSubEventHandler {
    Q_OBJECT

public:
    XmppPepExtension();

    void setOwnBareJid(const QString &jid);
    void setNodeName(const QString &nodeName);

    QStringList discoveryFeatures() const override;
    bool handlePubSubEvent(const QDomElement &element, const QString &pubSubService, const QString &nodeName) override;

signals:
    void notePublished(const QtNote::XmppRemoteNote &note);
    void noteRetracted(const QString &id);
    void nodeInvalidated();
    void malformedItem(const QString &error);

private:
    QString ownBareJid_;
    QString nodeName_;
};

} // namespace QtNote

#endif // XMPPPEPEXTENSION_H
