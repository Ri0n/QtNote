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

#ifndef QTNOTE_H
#define QTNOTE_H

#include <QObject>

class QSystemTrayIcon;
class QAction;
class QMenu;

namespace QtNote {

class ShortcutsManager;
class NoteWidget;
class TrayIconInterface;

class Main : public QObject
{
	Q_OBJECT
public:
	explicit Main(QObject *parent = 0);
	inline bool isOperable() const { return inited_; }
	NoteWidget *noteWidget(const QString &storageId, const QString &noteId);
public slots:
	void showNoteDialog(const QString &storageId, const QString &noteId = QString::null, const QString &contents = QString::null);
	void notifyError(const QString &);
	inline ShortcutsManager* shortcutsManager() const { return _shortcutsManager; }

private:
	class Private;
	Private *d;

	bool inited_;
	QSystemTrayIcon *tray;
	QMenu *contextMenu;
	QAction *actQuit, *actNew, *actAbout, *actOptions, *actManager;
	ShortcutsManager* _shortcutsManager;

	void parseAppArguments(const QStringList &args);
private slots:
	void showNoteList(int);
	void exitQtNote();
	void showAbout();
	void showNoteManager();
	void showOptions();
	void createNewNote();
	void createNewNoteFromSelection();
	void appMessageReceived(const QByteArray &msg);	
	void note_trashRequested();
	void note_saveRequested();
	void note_invalidated();
};

} // namespace QtNote

#endif // QTNOTE_H
