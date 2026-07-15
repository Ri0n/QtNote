#ifndef XMPPSETTINGSWIDGET_H
#define XMPPSETTINGSWIDGET_H

#include "xmppdto.h"

#include <QWidget>

class QLineEdit;
class QLabel;
class QComboBox;
class QPushButton;
class QSpinBox;

namespace QtNote {

/**
 * @brief Settings and maintenance UI for an XMPP storage account.
 *
 * The widget only gathers configuration and emits user intentions. Keychain,
 * OMEMO, repair, and key-sync operations are performed by XmppStorage/backend,
 * keeping protocol lifetimes outside the QWidget.
 */
class XmppSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    explicit XmppSettingsWidget(const XmppConfig &config, QWidget *parent = nullptr);

    XmppConfig config() const;
    void       setKeyState(const QByteArray &keyId, const QString &message = {});
    void       setRecoveryKey(const QString &key);
    void setOmemoDevices(const XmppDeviceInfo &ownDevice, bool ownBundleValid, const QList<XmppDeviceInfo> &devices,
                         const QString &message = {});

signals:
    void apply();
    void createKeyRequested(const QString &jid);
    void importKeyRequested(const QString &jid, const QString &recoveryKey);
    void exportKeyRequested(const QString &jid);
    void omemoSyncRequested(const QString &jid);
    void omemoDevicesRequested(const QString &jid);
    void trustOmemoDeviceRequested(const QString &jid, const QByteArray &keyId);
    void repairOmemoDeviceRequested(const QString &jid);

private:
    /// Stable installation origin preserved when editing visible settings.
    QString      originId_;
    QLineEdit   *jid_;
    QLineEdit   *password_;
    QLineEdit   *host_;
    QSpinBox    *port_;
    QLineEdit   *resource_;
    QLineEdit   *nodeName_;
    QSpinBox    *timeoutSeconds_;
    QLabel      *keyState_;
    QLineEdit   *recoveryKey_;
    QLabel      *ownOmemoDevice_;
    QPushButton *repairOmemoDevice_;
    QComboBox   *omemoDevices_;
    QPushButton *trustDevice_;
    /// Maps omemoDevices_ rows to binary fingerprints without displaying raw data.
    QList<QByteArray> omemoDeviceKeys_;
};

} // namespace QtNote

#endif // XMPPSETTINGSWIDGET_H
