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

#include <QFileDialog>

#include "ptfstoragesettingswidget.h"
#include "ui_ptfstoragesettingswidget.h"

PTFStorageSettingsWidget::PTFStorageSettingsWidget(const QString &path, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PTFStorageSettingsWidget)
{
	ui->setupUi(this);
	ui->lePath->setText(path);
	ui->lePath->setCursorPosition(0);
}

PTFStorageSettingsWidget::~PTFStorageSettingsWidget()
{
	delete ui;
}

QString PTFStorageSettingsWidget::path() const
{
	return ui->lePath->text();
}

void PTFStorageSettingsWidget::on_btnBrowse_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Choose storage directory"), ui->lePath->text());
	if (!dir.isEmpty()) {
		ui->lePath->setText(dir);
	}
}
