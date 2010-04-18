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

#include "mainwidget.h"

#include <QtGui/QApplication>
#include "notemanager.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
	actQuit = new QAction(QIcon(":/icons/exit"), "&Quit", this);
	actNew = new QAction(QIcon(":/icons/new"), "&New", this);

	contextMenu = new QMenu(this);
	contextMenu->addAction(actNew);
	contextMenu->addSeparator();
	contextMenu->addAction(actQuit);


	tray = new QSystemTrayIcon(this);
	tray->setIcon(QIcon(":/icons/trayicon"));
	tray->show();
	tray->setContextMenu(contextMenu);

	connect(actQuit, SIGNAL(triggered()), this, SLOT(exitQtNote()));
	connect(actNew, SIGNAL(triggered()), this, SLOT(createNewNote()));
	connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
			SLOT(showNoteList(QSystemTrayIcon::ActivationReason)));
}

Widget::~Widget()
{
}

void Widget::exitQtNote()
{
	QApplication::quit();
}

void Widget::showNoteDialog(const QString &storageId, const QString &noteId)
{
	Note *note = 0;
	NoteDialog *dlg = 0;
	if (!noteId.isEmpty()) {
		note = NoteManager::instance()->getNote(storageId, noteId);
		if (!note) {
			qWarning("faield to load note: %s", qPrintable(noteId));
			return;
		}
		// check if dialog for given storage and id is already opened
		foreach (NoteDialog *d, findChildren<NoteDialog *>("noteDlg")) {
			if (d->checkOwnership(storageId, noteId)) {
				dlg = d;
				break;
			}
		}
	}

	if (!dlg) {
		dlg = new NoteDialog(this, storageId, noteId);
		dlg->setObjectName("noteDlg");
		dlg->setWindowIcon(QIcon(":/icons/pen"));
		connect(dlg, SIGNAL(saveRequested(QString,QString,QString)),
				SLOT(onSaveNote(QString,QString,QString)));
		connect(dlg, SIGNAL(trashRequested(QString,QString)),
				SLOT(onDeleteNote(QString,QString)));
	}
	if (note) {
		dlg->setText(note->text());
		dlg->setWindowTitle(note->title());
	}
	dlg->show();
	dlg->raise();
	dlg->activateWindow();
}

void Widget::showNoteList(QSystemTrayIcon::ActivationReason reason)
{
	if (reason != QSystemTrayIcon::Trigger) {
		return;
	}
	QMenu menu(this);
	menu.addAction(actNew);
	menu.addSeparator();
	QList<NoteListItem> notes = NoteManager::instance()->noteList();
	for (int i=0; i<notes.count(); i++) {
		menu.addAction(notes[i].title)->setData(i);;
	}
	const QPoint menuPos = tray->geometry().bottomLeft();
	QAction *act = menu.exec(tray->geometry().bottomLeft(), actNew);
	if (act && act != actNew) {
		NoteListItem &note = notes[act->data().toInt()];
		showNoteDialog(note.storageId, note.id);
	}
}

void Widget::createNewNote()
{
	showNoteDialog(NoteManager::instance()->defaultStorage()->systemName());
}

void Widget::onSaveNote(const QString &storageId, const QString &noteId,
						const QString &text)
{
	NoteStorage *storage = NoteManager::instance()->storage(storageId);
	if (noteId.isEmpty()) {
		storage->createNote(text);
	} else {
		if (NoteManager::instance()->getNote(storageId, noteId)->text() != text) {
			storage->saveNote(noteId, text);
		}
	}
}

void Widget::onDeleteNote(const QString &storageId, const QString &noteId)
{
	NoteStorage *storage = NoteManager::instance()->storage(storageId);
	storage->deleteNote(noteId);
}
