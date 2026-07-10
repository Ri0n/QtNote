#include "nextcloudsettingswidget.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QUrl>
#include <QVBoxLayout>

namespace QtNote {

NextcloudSettingsWidget::NextcloudSettingsWidget(const NextcloudConfig &config, QWidget *parent) :
    QWidget(parent), serverUrl_(new QLineEdit(this)), userName_(new QLineEdit(this)), appPassword_(new QLineEdit(this)),
    timeoutSeconds_(new QSpinBox(this))
{
    serverUrl_->setText(config.serverUrl.toString(QUrl::RemoveQuery | QUrl::RemoveFragment));
    serverUrl_->setPlaceholderText(QStringLiteral("https://cloud.example.com"));
    userName_->setText(config.userName);
    appPassword_->setText(config.appPassword);
    appPassword_->setEchoMode(QLineEdit::Password);

    timeoutSeconds_->setRange(2, 120);
    timeoutSeconds_->setSuffix(tr(" s"));
    timeoutSeconds_->setValue(qBound(2, config.timeoutMs / 1000, 120));

    auto *form = new QFormLayout;
    form->addRow(tr("Server URL:"), serverUrl_);
    form->addRow(tr("User name:"), userName_);
    form->addRow(tr("App password:"), appPassword_);
    form->addRow(tr("Request timeout:"), timeoutSeconds_);

    auto *hint
        = new QLabel(tr("Use a Nextcloud app password, especially when two-factor authentication is enabled."), this);
    hint->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(hint);
    layout->addStretch();
}

NextcloudConfig NextcloudSettingsWidget::config() const
{
    NextcloudConfig result;
    result.serverUrl   = QUrl::fromUserInput(serverUrl_->text().trimmed());
    result.userName    = userName_->text().trimmed();
    result.appPassword = appPassword_->text();
    result.timeoutMs   = timeoutSeconds_->value() * 1000;
    return result;
}

} // namespace QtNote
