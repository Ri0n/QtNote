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

#include <QApplication>
#include <QDesktopWidget>
#include <QStyle>
#include <QSettings>
#include <QMessageBox>
#include <QtSingleApplication>
#include <QDebug>
#include <QProcess>
#include <QClipboard>

#include "notemanager.h"
#include "notedialog.h"
#include "aboutdlg.h"
#include "optionsdlg.h"
#include "notemanagerdlg.h"
#include "utils.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
	actQuit = new QAction(QIcon(":/icons/exit"), tr("&Quit"), this);
	actNew = new QAction(QIcon(":/icons/new"), tr("&New"), this);
	actAbout = new QAction(QIcon(":/icons/trayicon"), tr("&About"), this);
	actOptions = new QAction(QIcon(":/icons/options"), tr("&Options"), this);
	actManager = new QAction(QIcon(":/icons/manager"), tr("&Note Manager"), this);

	contextMenu = new QMenu(this);
	contextMenu->addAction(actNew);
	contextMenu->addSeparator();
	contextMenu->addAction(actManager);
	contextMenu->addAction(actOptions);
	contextMenu->addAction(actAbout);
	contextMenu->addSeparator();
	contextMenu->addAction(actQuit);


	tray = new QSystemTrayIcon(this);
	tray->setIcon(QIcon(":/icons/trayicon"));
	tray->show();
	tray->setContextMenu(contextMenu);

	connect(QtSingleApplication::instance(), SIGNAL(messageReceived(const QByteArray &)), SLOT(appMessageReceived(const QByteArray &)));
	connect(actQuit, SIGNAL(triggered()), this, SLOT(exitQtNote()));
	connect(actNew, SIGNAL(triggered()), this, SLOT(createNewNote()));
	connect(actManager, SIGNAL(triggered()), this, SLOT(showNoteManager()));
	connect(actOptions, SIGNAL(triggered()), this, SLOT(showOptions()));
	connect(actAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
	connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
			SLOT(showNoteList(QSystemTrayIcon::ActivationReason)));

	parseAppArguments(QtSingleApplication::instance()->arguments().mid(1));
}

Widget::~Widget()
{
}

void Widget::parseAppArguments(const QStringList &args)
{
	if (args.isEmpty()) {
		return;
	}
	int i = 0;
	while (i < args.size()) {
		if (args.at(i) == QLatin1String("-n")) {
			if (i < args.size() + 1 && args.at(i + 1)[0] != '-') {
				i++;
				if (args.at(i) == QLatin1String("selection")) {
					createNewNoteFromSelection();
				}
			} else {
				createNewNote();
			}
		}
		i++;
	}
}

void Widget::exitQtNote()
{
	QApplication::quit();
}

void Widget::appMessageReceived(const QByteArray &msg)
{
	QDataStream stream(msg);
	QStringList params;
	stream >> params;
	parseAppArguments(params);
}

void Widget::showAbout()
{
	AboutDlg *d = new AboutDlg(this);
	d->setAttribute(Qt::WA_DeleteOnClose);
	d->show();
}

void Widget::showNoteManager()
{
	NoteManagerDlg *d = new NoteManagerDlg(this);
	d->setWindowIcon(QIcon(":/icons/manager"));
	d->setAttribute(Qt::WA_DeleteOnClose);
	d->show();
}

void Widget::showOptions()
{
	OptionsDlg *d = new OptionsDlg(this);
	d->setAttribute(Qt::WA_DeleteOnClose);
	d->show();
}

void Widget::showNoteDialog(const QString &storageId, const QString &noteId, const QString &contents)
{
	Note note;
	NoteDialog *dlg = 0;
	if (!noteId.isEmpty()) {
		note = NoteManager::instance()->getNote(storageId, noteId);
		if (note.isNull()) {
			qWarning("failed to load note: %s", qPrintable(noteId));
			return;
		}
		// check if dialog for given storage and id is already opened
		dlg = NoteDialog::findDialog(storageId, noteId);
	}

	if (!dlg) {
		dlg = new NoteDialog(this, storageId, noteId);
		dlg->setObjectName("noteDlg");
		dlg->setWindowIcon(QIcon(":/icons/trayicon"));
		dlg->setAcceptRichText(NoteManager::instance()->storage(storageId)
							   ->isRichTextAllowed());
		connect(dlg, SIGNAL(save()), SLOT(onSaveNote()));
		connect(dlg, SIGNAL(trash()), SLOT(onDeleteNote()));
		if (!note.isNull()) {
			if (note.text().startsWith(note.title())) {
				dlg->setText(note.text());
			} else {
				dlg->setText(note.title() + "\n" + note.text());
			}
		} else if (contents.size()) {
			dlg->setText(contents);
		}
	}
	dlg->show();
	dlg->activateWindow();
	dlg->raise();
}

void Widget::showNoteList(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::MiddleClick || reason == QSystemTrayIcon::DoubleClick) {
		createNewNote();
		return;
	}
	if (reason != QSystemTrayIcon::Trigger) {
		return;
	}
	QMenu menu(this);
	menu.addAction(actNew);
	menu.addSeparator();
	QSettings s;
	QList<NoteListItem> notes = NoteManager::instance()->noteList(
								s.value("ui.menu-notes-amount", 15).toInt());
	for (int i=0; i<notes.count(); i++) {
		menu.addAction(menu.style()->standardIcon(
			QStyle::SP_MessageBoxInformation),
			Utils::cuttedDots(notes[i].title, 48).replace('&', "&&")
		)->setData(i);
	}
	menu.show();
	menu.activateWindow();
	menu.raise();
	QRect dr = QApplication::desktop()->geometry();
	QRect ir = tray->geometry();
	QRect mr = menu.geometry();
	mr.setSize(menu.sizeHint());
	if (ir.left() < dr.width()/2) {
		if (ir.top() < dr.height()/2) {
			mr.moveTopLeft(ir.bottomLeft());
		} else {
			mr.moveBottomLeft(ir.topLeft());
		}
	} else {
		if (ir.top() < dr.height()/2) {
			mr.moveTopRight(ir.bottomRight());
		} else {
			mr.moveBottomRight(ir.topRight());
		}
	}
	QAction *act = menu.exec(mr.topLeft());
	if (act && act != actNew) {
		NoteListItem &note = notes[act->data().toInt()];
		showNoteDialog(note.storageId, note.id);
	}
}

void Widget::createNewNote()
{
	showNoteDialog(NoteManager::instance()->defaultStorage()->systemName());
}

void Widget::createNewNoteFromSelection()
{
	QString contents;
#ifdef Q_OS_UNIX
	contents = QApplication::clipboard()->text(QClipboard::Selection);
#endif
#ifdef Q_OS_WIN
	int n = 0;
	QVector<INPUT> input(10);
	memset(input.data(), 0, input.size()*sizeof(INPUT));

	if (GetAsyncKeyState(VK_MENU) & (1 < 15)) {
		input[n].ki.dwFlags = KEYEVENTF_KEYUP;
		input[n].ki.wVk = VK_MENU;
		input[n++].ki.wScan = MapVirtualKey( VK_MENU, MAPVK_VK_TO_VSC );
	}
	if (GetAsyncKeyState(VK_SHIFT) & (1 < 15)) {
		input[n].ki.dwFlags = KEYEVENTF_KEYUP;
		input[n].ki.wVk = VK_SHIFT;
		input[n++].ki.wScan = MapVirtualKey(VK_SHIFT, MAPVK_VK_TO_VSC);
	}
	input[n].ki.wVk = VK_CONTROL;
	input[n].ki.dwFlags = 0;
	input[n++].ki.wScan = MapVirtualKey( VK_CONTROL, MAPVK_VK_TO_VSC );

	input[n].ki.wVk = 0x43; // Virtual key code for 'c'
	input[n].ki.dwFlags = 0;
	input[n++].ki.wScan = MapVirtualKey( 0x56, MAPVK_VK_TO_VSC );

	input[n].ki.dwFlags = KEYEVENTF_KEYUP;
	input[n].ki.wVk = input[0].ki.wVk;
	input[n++].ki.wScan = input[0].ki.wScan;

	input[n].ki.dwFlags = KEYEVENTF_KEYUP;
	input[n].ki.wVk = input[1].ki.wVk;
	input[n++].ki.wScan = input[1].ki.wScan;

	bool sent = true;
	for (int i = 0; i < n; i++) {
		input[i].type = INPUT_KEYBOARD;
		if(!SendInput(1, (LPINPUT)&(input[i]), sizeof(INPUT) ) ) {
			sent = false;
			break;
		}
		Sleep(30);
	}
	if (sent) {
		contents = QApplication::clipboard()->text();
	}
#endif
	if (contents.size()) {
		showNoteDialog(NoteManager::instance()->defaultStorage()->systemName(), QString(), contents);
	}
}

void Widget::onSaveNote()
{
	NoteDialog *dlg = static_cast<NoteDialog *>(sender());
	QString storageId = dlg->storageId();
	QString noteId = dlg->noteId();
	QString text = dlg->text();
	NoteStorage *storage = NoteManager::instance()->storage(storageId);
	if (text.isEmpty()) { // delete empty note
		if (!noteId.isEmpty()) {
			storage->deleteNote(noteId);
		}
		return;
	}
	if (noteId.isEmpty()) {
		noteId = storage->createNote(text);
		dlg->setNoteId(noteId);
	} else {
		if (NoteManager::instance()->getNote(storageId, noteId).text() != text)
		{
			storage->saveNote(noteId, text);
		}
	}
}

void Widget::onDeleteNote()
{
	NoteDialog *dlg = static_cast<NoteDialog *>(sender());
	QSettings s;
	NoteStorage *storage = NoteManager::instance()->storage(dlg->storageId());
	if (!dlg->text().isEmpty() && s.value("ui.ask-on-delete", true).toBool() &&
		QMessageBox::question(this, tr("Deletion confirmation"),
							  tr("Are you sure want to delete this note?"),
							  QMessageBox::Yes | QMessageBox::No) !=
		QMessageBox::Yes) {
		return;
	}
	storage->deleteNote(dlg->noteId());
}
