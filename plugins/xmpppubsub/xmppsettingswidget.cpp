#include "xmppsettingswidget.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QVBoxLayout>

namespace QtNote {

XmppSettingsWidget::XmppSettingsWidget(const XmppConfig &config, QWidget *parent) :
    QWidget(parent), originId_(config.originId), jid_(new QLineEdit(this)), password_(new QLineEdit(this)),
    host_(new QLineEdit(this)), port_(new QSpinBox(this)), resource_(new QLineEdit(this)),
    nodeName_(new QLineEdit(this)), timeoutSeconds_(new QSpinBox(this)), keyState_(new QLabel(this)),
    recoveryKey_(new QLineEdit(this)), ownOmemoDevice_(new QLabel(tr("Not initialized"), this)),
    omemoDevices_(new QComboBox(this)), trustDevice_(new QPushButton(tr("Trust selected device"), this))
{
    jid_->setText(config.jid);
    jid_->setPlaceholderText(QStringLiteral("user@example.org"));

    password_->setText(config.password);
    password_->setEchoMode(QLineEdit::Password);

    host_->setText(config.host);
    host_->setPlaceholderText(tr("leave empty to use DNS SRV"));

    port_->setRange(0, 65535);
    port_->setSpecialValueText(tr("automatic"));
    port_->setValue(config.port);

    resource_->setText(config.resource);
    resource_->setPlaceholderText(QStringLiteral("QtNote-device"));

    nodeName_->setText(config.nodeName);
    nodeName_->setPlaceholderText(QStringLiteral("urn:xmpp:qtnote:notes"));

    timeoutSeconds_->setRange(2, 120);
    timeoutSeconds_->setSuffix(tr(" s"));
    timeoutSeconds_->setValue(qBound(2, config.timeoutMs / 1000, 120));

    omemoDevices_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    omemoDevices_->setMinimumContentsLength(48);
    omemoDevices_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->addRow(tr("JID:"), jid_);
    form->addRow(tr("Password:"), password_);
    form->addRow(tr("Host override:"), host_);
    form->addRow(tr("Port:"), port_);
    form->addRow(tr("Resource:"), resource_);
    form->addRow(tr("PEP node:"), nodeName_);
    form->addRow(tr("Operation timeout:"), timeoutSeconds_);

    recoveryKey_->setEchoMode(QLineEdit::Password);
    recoveryKey_->setPlaceholderText(QStringLiteral("qtnote-key-v1:…"));
    auto *createKey      = new QPushButton(tr("Create key"), this);
    auto *importKey      = new QPushButton(tr("Import"), this);
    auto *exportKey      = new QPushButton(tr("Export"), this);
    auto *omemoSync      = new QPushButton(tr("Sync via OMEMO"), this);
    auto *refreshDevices = new QPushButton(tr("Refresh devices"), this);
    auto *keyButtons     = new QHBoxLayout;
    keyButtons->addWidget(createKey);
    keyButtons->addWidget(importKey);
    keyButtons->addWidget(exportKey);
    keyButtons->addWidget(omemoSync);
    form->addRow(tr("Storage key:"), keyState_);
    form->addRow(tr("Recovery key:"), recoveryKey_);
    form->addRow(QString(), keyButtons);
    auto *deviceButtons = new QHBoxLayout;
    deviceButtons->addWidget(refreshDevices);
    deviceButtons->addWidget(trustDevice_);
    ownOmemoDevice_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ownOmemoDevice_->setWordWrap(true);
    form->addRow(tr("This OMEMO device:"), ownOmemoDevice_);
    form->addRow(tr("Other OMEMO devices:"), omemoDevices_);
    form->addRow(QString(), deviceButtons);

    connect(createKey, &QPushButton::clicked, this,
            [this]() { emit createKeyRequested(jid_->text().trimmed().section(QLatin1Char('/'), 0, 0)); });
    connect(importKey, &QPushButton::clicked, this, [this]() {
        emit importKeyRequested(jid_->text().trimmed().section(QLatin1Char('/'), 0, 0), recoveryKey_->text());
    });
    connect(exportKey, &QPushButton::clicked, this,
            [this]() { emit exportKeyRequested(jid_->text().trimmed().section(QLatin1Char('/'), 0, 0)); });
    connect(omemoSync, &QPushButton::clicked, this,
            [this]() { emit omemoSyncRequested(jid_->text().trimmed().section(QLatin1Char('/'), 0, 0)); });
    connect(refreshDevices, &QPushButton::clicked, this,
            [this]() { emit omemoDevicesRequested(jid_->text().trimmed().section(QLatin1Char('/'), 0, 0)); });
    trustDevice_->setEnabled(false);
    connect(trustDevice_, &QPushButton::clicked, this, [this]() {
        const auto index = omemoDevices_->currentIndex();
        if (index < 0 || index >= omemoDeviceKeys_.size())
            return;
        emit trustOmemoDeviceRequested(jid_->text().trimmed().section(QLatin1Char('/'), 0, 0),
                                       omemoDeviceKeys_.at(index));
    });

    auto *privacy = new QLabel(tr("Index metadata and note contents are end-to-end encrypted with independent "
                                  "derived keys. The recovery key grants access to the complete XMPP storage."),
                               this);
    privacy->setWordWrap(true);

    auto *tls = new QLabel(tr("TLS is mandatory. Host and port should normally remain automatic."), this);
    tls->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(privacy);
    layout->addWidget(tls);
    layout->addStretch();
}

void XmppSettingsWidget::setKeyState(const QByteArray &keyId, const QString &message)
{
    keyState_->setText(!message.isEmpty()    ? message
                           : keyId.isEmpty() ? tr("Not configured")
                                             : QString::fromLatin1(keyId.left(8).toHex()));
}

void XmppSettingsWidget::setRecoveryKey(const QString &key)
{
    recoveryKey_->setText(key);
    recoveryKey_->setEchoMode(QLineEdit::Normal);
    recoveryKey_->selectAll();
}

void XmppSettingsWidget::setOmemoDevices(const XmppDeviceInfo &ownDevice, const QList<XmppDeviceInfo> &devices,
                                         const QString &message)
{
    if (ownDevice.deviceId) {
        const auto fingerprint
            = ownDevice.keyId.isEmpty() ? tr("fingerprint unavailable") : QString::fromLatin1(ownDevice.keyId.toHex());
        const auto label = ownDevice.label.isEmpty() ? tr("unnamed") : ownDevice.label;
        ownOmemoDevice_->setText(
            tr("%1 (OMEMO label) — ID %2\nIdentity key: %3").arg(label).arg(ownDevice.deviceId).arg(fingerprint));
        ownOmemoDevice_->setToolTip({});
    } else {
        ownOmemoDevice_->setText(tr("Not initialized"));
        ownOmemoDevice_->setToolTip({});
    }
    omemoDevices_->clear();
    omemoDeviceKeys_.clear();
    for (const auto &device : devices) {
        if (device.keyId.isEmpty())
            continue;
        const auto label = device.label.isEmpty() ? tr("Unnamed device") : device.label;
        omemoDevices_->addItem(QStringLiteral("OMEMO %1 — %2 — %3 — trust %4")
                                   .arg(device.deviceId)
                                   .arg(label, QString::fromLatin1(device.keyId.left(8).toHex()))
                                   .arg(device.trustLevel));
        omemoDeviceKeys_.append(device.keyId);
    }
    trustDevice_->setEnabled(omemoDevices_->currentIndex() >= 0);
    if (!message.isEmpty())
        keyState_->setText(message);
}

XmppConfig XmppSettingsWidget::config() const
{
    XmppConfig result;
    result.jid       = jid_->text().trimmed().section(QLatin1Char('/'), 0, 0);
    result.password  = password_->text();
    result.host      = host_->text().trimmed();
    result.port      = port_->value();
    result.resource  = resource_->text().trimmed();
    result.nodeName  = nodeName_->text().trimmed();
    result.originId  = originId_;
    result.timeoutMs = timeoutSeconds_->value() * 1000;
    return result;
}

} // namespace QtNote
