#include <QLocale>

#include "settingsdlg.h"
#include "ui_settingsdlg.h"

SettingsDlg::SettingsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDlg)
{
	ui->setupUi(this);

	QList<QLocale> allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
	foreach (auto locale, allLocales) {
		QListWidgetItem *item = new QListWidgetItem(locale.nativeLanguageName() + QLatin1String(" (") +
													locale.nativeCountryName() + QLatin1Char(')'));
		item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		item->setCheckState(Qt::Unchecked);
		ui->lwLangs->addItem(item);
	}
}

SettingsDlg::~SettingsDlg()
{
	delete ui;
}

QList<QLocale> SettingsDlg::preferredList() const
{
	return QList<QLocale>(); // TODO
}
