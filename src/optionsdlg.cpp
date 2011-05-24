/*
QtNote - Simple note-taking application
Copyright (C) 2010 Ili'nykh Sergey

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Contacts:
E-Mail: rion4ik@gmail.com XMPP: rion@jabber.ru
*/

#include "optionsdlg.h"
#include "ui_optionsdlg.h"
#include "notemanager.h"
#include <QStringListModel>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QDir>


class OptionsDlg::PriorityModel : public QStringListModel
{
private:
	QMap<QString, QString> titleMap;

public:

	PriorityModel(QObject *parent)
		: QStringListModel(parent)
	{
		QStringList orderedNames;

		foreach(StorageItem s,  NoteManager::instance()->prioritizedStorages())
		{
			titleMap[s.storage->systemName()] = s.storage->titleName();
			orderedNames.append(s.storage->titleName());
		}
		setStringList(orderedNames);
	}

	QStringList priorityList() const
	{
		QStringList ret;
		foreach (const QString &title, stringList()) {
			ret.append(titleMap.key(title));
		}
		return ret;
	}

	Qt::ItemFlags flags(const QModelIndex &index) const
	{
		Qt::ItemFlags defaultFlags = QStringListModel::flags(index);
		defaultFlags ^= (Qt::ItemIsDropEnabled | Qt::ItemIsEditable);

		if (index.isValid()) {
			return Qt::ItemIsDragEnabled | defaultFlags;
		} else {
			return defaultFlags | Qt::ItemIsDropEnabled;
		}
	}

	Qt::DropActions supportedDropActions() const
	{
		return Qt::MoveAction;
	}

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
	{
		if (index.isValid() && role == Qt::DecorationRole) {
			QString storageId = titleMap.key(stringList()[index.row()]);
			if (!storageId.isEmpty()) {
				return NoteManager::instance()->storage(storageId)->storageIcon();
			}
		}
		return QStringListModel::data(index, role);
	}
};


OptionsDlg::OptionsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDlg)
{
	ui->setupUi(this);
#ifdef Q_OS_LINUX
	QFile desktop(QDir::homePath() + "/.config/autostart/" APPNAME ".desktop");
	if (desktop.open(QIODevice::ReadOnly) && QString(desktop.readAll())
		.contains(QRegExp("\\bhidden\\s*=\\s*false", Qt::CaseInsensitive))) {
		ui->ckAutostart->setChecked(true);
	}
#elif defined(Q_OS_WIN)
	QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\"
				  "CurrentVersion\\Run", QSettings::NativeFormat);
	ui->ckAutostart->setChecked(
			reg.contains(QCoreApplication::applicationName()));
#else
	ui->ckAutostart->setVisible(false);
#endif
	priorityModel = new PriorityModel(this);
	ui->priorityView->setModel(priorityModel);
	QSettings s;
	ui->ckAskDel->setChecked(s.value("ui.ask-on-delete", true).toBool());
	ui->spMenuNotesAmount->setValue(s.value("ui.menu-notes-amount", 15).toInt());
	resize(0, 0);
}

OptionsDlg::~OptionsDlg()
{
    delete ui;
}

void OptionsDlg::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void OptionsDlg::accept()
{
	QStringList storageCodes = priorityModel->priorityList();
	NoteManager::instance()->updatePriorities(storageCodes);
	QSettings s;
	s.setValue("storage.priority", storageCodes);
	s.setValue("ui.ask-on-delete", ui->ckAskDel->isChecked());
	s.setValue("ui.menu-notes-amount", ui->spMenuNotesAmount->value());
#ifdef Q_OS_LINUX
	QDir home = QDir::home();
	if (!home.exists(".config/autostart")) {
		home.mkpath(".config/autostart");
	}
	QFile desktopFile(DATADIR "/applications/" APPNAME ".desktop");
	if (desktopFile.open(QIODevice::ReadOnly)) {
		QByteArray contents = desktopFile.readAll();
		QFile f(home.absolutePath() +
				"/.config/autostart/" APPNAME ".desktop");

		if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
			f.write(contents.trimmed());
			f.write(QString("\nHidden=%1").arg(ui->ckAutostart->isChecked()?
											   "false\n":"true\n").toUtf8());
		}
	}
#elif defined(Q_OS_WIN)
	QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\"
				  "CurrentVersion\\Run", QSettings::NativeFormat);
	if(ui->ckAutostart->isChecked())
		reg.setValue(QCoreApplication::applicationName(), '"' +
					 QDir::toNativeSeparators(QCoreApplication::
											  applicationFilePath()) + '"');
	else
		reg.remove(QCoreApplication::applicationName());
#endif
	QDialog::accept();
}
