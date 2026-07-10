#ifndef NEXTCLOUDSETTINGSWIDGET_H
#define NEXTCLOUDSETTINGSWIDGET_H

#include "nextclouddto.h"

#include <QWidget>

class QLineEdit;
class QSpinBox;

namespace QtNote {

class NextcloudSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    explicit NextcloudSettingsWidget(const NextcloudConfig &config, QWidget *parent = nullptr);

    NextcloudConfig config() const;

signals:
    void apply();

private:
    QLineEdit *serverUrl_;
    QLineEdit *userName_;
    QLineEdit *appPassword_;
    QSpinBox  *timeoutSeconds_;
};

} // namespace QtNote

#endif // NEXTCLOUDSETTINGSWIDGET_H
