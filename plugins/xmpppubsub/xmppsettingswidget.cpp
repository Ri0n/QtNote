#include "xmppsettingswidget.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace QtNote {

XmppSettingsWidget::XmppSettingsWidget(const XmppConfig &config, QWidget *parent) :
    QWidget(parent), originId_(config.originId), jid_(new QLineEdit(this)), password_(new QLineEdit(this)),
    host_(new QLineEdit(this)), port_(new QSpinBox(this)), resource_(new QLineEdit(this)),
    nodeName_(new QLineEdit(this)), timeoutSeconds_(new QSpinBox(this))
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
    nodeName_->setPlaceholderText(QStringLiteral("urn:xmpp:qtnote:notes:0"));

    timeoutSeconds_->setRange(2, 120);
    timeoutSeconds_->setSuffix(tr(" s"));
    timeoutSeconds_->setValue(qBound(2, config.timeoutMs / 1000, 120));

    auto *form = new QFormLayout;
    form->addRow(tr("JID:"), jid_);
    form->addRow(tr("Password:"), password_);
    form->addRow(tr("Host override:"), host_);
    form->addRow(tr("Port:"), port_);
    form->addRow(tr("Resource:"), resource_);
    form->addRow(tr("PEP node:"), nodeName_);
    form->addRow(tr("Operation timeout:"), timeoutSeconds_);

    auto *privacy = new QLabel(tr("The node is configured as persistent and allowlist-only. "
                                  "This first version is private from other XMPP users, but the note payload is not "
                                  "end-to-end encrypted and can be read by the XMPP server."),
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
