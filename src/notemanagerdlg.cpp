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

#include "notemanagerdlg.h"
#include "ui_notemanagerdlg.h"
#include "notemanagermodel.h"

NoteManagerDlg::NoteManagerDlg() :
	QDialog(0),
    ui(new Ui::NoteManagerDlg)
{
    ui->setupUi(this);
	model = new NoteManagerModel(this);
	ui->notesTree->setModel(model);
	int sumCount = 0;
	for (int i = 0, cnt = model->rowCount(); i < cnt; i++) {
		sumCount += model->rowCount(model->index(i, 0));
	}
	setWindowTitle(tr("Note Manager (%1)").arg(tr("%n notes", 0, sumCount)));
	connect(ui->notesTree, SIGNAL(doubleClicked(QModelIndex)), SLOT(itemDoubleClicked(QModelIndex)));
}

NoteManagerDlg::~NoteManagerDlg()
{
    delete ui;
}

void NoteManagerDlg::changeEvent(QEvent *e)
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

void NoteManagerDlg::itemDoubleClicked(const QModelIndex &index)
{
	QString noteId = model->data(index, NoteManagerModel::NoteId).toString();
	if (!noteId.isEmpty()) {
		emit showNoteRequested(model->data(index, NoteManagerModel::StorageId).toString(), noteId);
	}
}
