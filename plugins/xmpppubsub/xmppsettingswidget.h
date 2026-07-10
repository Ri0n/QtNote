#ifndef XMPPSETTINGSWIDGET_H
#define XMPPSETTINGSWIDGET_H

#include "xmppdto.h"

#include <QWidget>

class QLineEdit;
class QSpinBox;

namespace QtNote {

class XmppSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    explicit XmppSettingsWidget(const XmppConfig &config, QWidget *parent = nullptr);

    XmppConfig config() const;

signals:
    void apply();

private:
    QString    originId_;
    QLineEdit *jid_;
    QLineEdit *password_;
    QLineEdit *host_;
    QSpinBox  *port_;
    QLineEdit *resource_;
    QLineEdit *nodeName_;
    QSpinBox  *timeoutSeconds_;
};

} // namespace QtNote

#endif // XMPPSETTINGSWIDGET_H
