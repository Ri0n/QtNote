#ifndef XMPPSETTINGSWIDGET_H
#define XMPPSETTINGSWIDGET_H

#include "xmppdto.h"

#include <QWidget>

class QLineEdit;
class QLabel;
class QComboBox;
class QSpinBox;

namespace QtNote {

class XmppSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    explicit XmppSettingsWidget(const XmppConfig &config, QWidget *parent = nullptr);

    XmppConfig config() const;
    void       setKeyState(const QByteArray &keyId, const QString &message = {});
    void       setRecoveryKey(const QString &key);
    void       setOmemoDevices(const QList<XmppDeviceInfo> &devices, const QString &message = {});

signals:
    void apply();
    void createKeyRequested(const QString &jid);
    void importKeyRequested(const QString &jid, const QString &recoveryKey);
    void exportKeyRequested(const QString &jid);
    void omemoSyncRequested(const QString &jid);
    void omemoDevicesRequested(const QString &jid);
    void trustOmemoDeviceRequested(const QString &jid, const QByteArray &keyId);

private:
    QString    originId_;
    QLineEdit *jid_;
    QLineEdit *password_;
    QLineEdit *host_;
    QSpinBox  *port_;
    QLineEdit *resource_;
    QLineEdit *nodeName_;
    QSpinBox  *timeoutSeconds_;
    QLabel    *keyState_;
    QLineEdit *recoveryKey_;
    QComboBox *omemoDevices_;
};

} // namespace QtNote

#endif // XMPPSETTINGSWIDGET_H
