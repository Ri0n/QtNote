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

#ifndef NOTEDIALOG_H
#define NOTEDIALOG_H

#include <QDialog>

namespace Ui {
    class NoteDialog;
}

class NoteWidget;

class NoteDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(NoteDialog)
public:
	explicit NoteDialog(NoteWidget *noteWidget);
    virtual ~NoteDialog();

	static NoteDialog* findDialog(const QString &storageId, const QString &noteId);

protected:
	void changeEvent(QEvent *e);

private slots:
	void trashRequested();
	void noteIdChanged(const QString &oldId, const QString &newId);
	void firstLineChanged();
private:
	bool _trashRequested;
    Ui::NoteDialog *m_ui;
	NoteWidget *noteWidget;

	static QHash< QPair<QString,QString>, NoteDialog* > dialogs;



public slots:
	void done(int r);


};

#endif // NOTEDIALOG_H
