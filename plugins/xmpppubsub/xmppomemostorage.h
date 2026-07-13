#ifndef XMPPOMEMOSTORAGE_H
#define XMPPOMEMOSTORAGE_H

#include <QXmppOmemoStorage.h>

namespace QtNote {

class XmppOmemoStorage final : public QXmppOmemoStorage {
public:
    XmppOmemoStorage(QString path, QByteArray encryptionKey, QString accountId);

    bool       isValid() const { return error_.isEmpty(); }
    QString    errorString() const { return error_; }
    uint32_t   ownDeviceId() const { return data_.ownDevice ? data_.ownDevice->id : 0; }
    QString    ownDeviceLabel() const { return data_.ownDevice ? data_.ownDevice->label : QString {}; }
    QByteArray ownIdentityKey() const { return data_.ownDevice ? data_.ownDevice->publicIdentityKey : QByteArray {}; }
    QXmppTask<void> removeAllDevices();

    QXmppTask<OmemoData> allData() override;
    QXmppTask<void>      setOwnDevice(const std::optional<OwnDevice> &device) override;
    QXmppTask<void>      addSignedPreKeyPair(uint32_t keyId, const SignedPreKeyPair &keyPair) override;
    QXmppTask<void>      removeSignedPreKeyPair(uint32_t keyId) override;
    QXmppTask<void>      addPreKeyPairs(const QHash<uint32_t, QByteArray> &keyPairs) override;
    QXmppTask<void>      removePreKeyPair(uint32_t keyId) override;
    QXmppTask<void>      addDevice(const QString &jid, uint32_t deviceId, const Device &device) override;
    QXmppTask<void>      removeDevice(const QString &jid, uint32_t deviceId) override;
    QXmppTask<void>      removeDevices(const QString &jid) override;
    QXmppTask<void>      resetAll() override;

private:
    void load();
    void persist();

    QString    path_;
    QByteArray encryptionKey_;
    QString    accountId_;
    OmemoData  data_;
    QString    error_;
};

} // namespace QtNote

#endif // XMPPOMEMOSTORAGE_H
