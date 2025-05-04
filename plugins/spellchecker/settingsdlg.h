#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include "spellcheckplugin.h"

#include <QDialog>

namespace Ui {
class SettingsDlg;
}

namespace QtNote {

class SpellCheckPlugin;

class SettingsDlg : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDlg(SpellCheckPlugin *plugin, QWidget *parent = nullptr);
    ~SettingsDlg();

    QList<QLocale> preferredList() const;

private:
    Ui::SettingsDlg              *ui;
    QList<SpellCheckPlugin::Dict> dicts_;
};

}

#endif // SETTINGSDLG_H
