#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include <QDialog>

namespace Ui {
class SettingsDlg;
}

namespace QtNote {

class SpellCheckPlugin;

class SettingsDlg : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDlg(SpellCheckPlugin *plugin, QWidget *parent = 0);
    ~SettingsDlg();

    QList<QLocale> preferredList() const;

private:
    Ui::SettingsDlg *ui;
};

}

#endif // SETTINGSDLG_H
