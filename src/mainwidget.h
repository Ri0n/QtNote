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

#ifndef WIDGET_H
#define WIDGET_H

#include <QtGui/QWidget>
#include <QSystemTrayIcon>
#include <QAction>
#include <QMenu>

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();
	void showNoteDialog(const QString &storageId, const QString &noteId = "");

private:
	QSystemTrayIcon *tray;
	QMenu *contextMenu;
	QAction *actQuit, *actNew, *actAbout, *actOptions, *actManager;

private slots:
	void showNoteList(QSystemTrayIcon::ActivationReason);
	void exitQtNote();
	void showAbout();
	void showNoteManager();
	void showOptions();
	void createNewNote();
	void onSaveNote(const QString &storageId, const QString &noteId,
					const QString &text);
	void onDeleteNote(const QString &storageId, const QString &noteId);
};

#endif // WIDGET_H
