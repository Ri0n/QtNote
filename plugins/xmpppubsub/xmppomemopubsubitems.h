#ifndef XMPPOMEMOPUBSUBITEMS_H
#define XMPPOMEMOPUBSUBITEMS_H

#include <QList>
#include <QString>
#include <QXmppPubSubBaseItem.h>

namespace QtNote {

struct XmppOmemoListedDevice {
    QString label;
    QString id;
    QString labelSignature;
};

class XmppOmemoDeviceListItem final : public QXmppPubSubBaseItem {
public:
    static bool                         isItem(const QDomElement &element);
    const QList<XmppOmemoListedDevice> &devices() const { return devices_; }
    void setDevices(QList<XmppOmemoListedDevice> devices) { devices_ = std::move(devices); }

protected:
    void parsePayload(const QDomElement &element) override;
    void serializePayload(QXmlStreamWriter *writer) const override;

private:
    QList<XmppOmemoListedDevice> devices_;
};

class XmppOmemoBundleItem final : public QXmppPubSubBaseItem {
public:
    static bool       isItem(const QDomElement &element);
    const QByteArray &identityKey() const { return identityKey_; }

protected:
    void parsePayload(const QDomElement &element) override;

private:
    QByteArray identityKey_;
};

} // namespace QtNote

#endif // XMPPOMEMOPUBSUBITEMS_H
