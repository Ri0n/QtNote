#include <QLocale>

#include "engineinterface.h"
#include "settingsdlg.h"
#include "spellcheckplugin.h"
#include "ui_settingsdlg.h"

namespace QtNote {

class LocaleWidgetItem : public QListWidgetItem {
public:
    QLocale locale;

    LocaleWidgetItem(const QLocale &locale) :
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QListWidgetItem(locale.nativeLanguageName() + QLatin1String(" (") + locale.nativeTerritoryName()
#else
        QListWidgetItem(locale.nativeLanguageName() + QLatin1String(" (") + locale.nativeCountryName()
#endif
                        + QLatin1Char(')')),
        locale(locale)
    {
    }
};

SettingsDlg::SettingsDlg(SpellCheckPlugin *plugin, QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDlg)
{
    ui->setupUi(this);

    auto engine         = plugin->engine();
    auto preferredLangs = plugin->preferredLanguages();

    foreach (auto locale, engine->supportedLanguages()) {
        LocaleWidgetItem *item = new LocaleWidgetItem(locale);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setCheckState(preferredLangs.contains(locale) ? Qt::Checked : Qt::Unchecked);
        ui->lwLangs->addItem(item);
    }
}

SettingsDlg::~SettingsDlg() { delete ui; }

QList<QLocale> SettingsDlg::preferredList() const
{
    QList<QLocale> ret;
    for (int i = 0; i < ui->lwLangs->count(); i++) {
        auto item = (LocaleWidgetItem *)ui->lwLangs->item(i);
        if (item->checkState() == Qt::Checked) {
            ret.append(item->locale);
        }
    }
    return ret;
}

} // namespace QtNote
