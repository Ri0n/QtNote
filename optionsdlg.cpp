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

	QStringList priorityList() const {
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
};


OptionsDlg::OptionsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDlg)
{
	ui->setupUi(this);
	priorityModel = new PriorityModel(this);
	ui->priorityView->setModel(priorityModel);
	QSettings s;
	ui->ckAskDel->setChecked(s.value("ui.ask-on-delete", true).toBool());
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
	QDialog::accept();
}
