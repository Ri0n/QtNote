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
#include <QTextCharFormat>
#include <QTimer>

namespace Ui {
    class NoteDialog;
}

class TypeAheadFindBar;

class NoteDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(NoteDialog)
public:
	explicit NoteDialog(const QString &storageId, const QString &noteId);
    virtual ~NoteDialog();
	void setText(QString text);
	QString text();
	void setAcceptRichText(bool state);
	inline QString storageId() const { return storageId_; }
	inline QString noteId() const { return noteId_; }
	void setNoteId(const QString &noteId);

	static NoteDialog* findDialog(const QString &storageId, const QString &noteId);

protected:
	void changeEvent(QEvent *e);
	void keyPressEvent(QKeyEvent *event);

private:
    Ui::NoteDialog *m_ui;
	TypeAheadFindBar *findBar;
	QTextCharFormat titleCharFormat_;
	QTextCharFormat secondLineCharFormat_;
	QString storageId_;
	QString noteId_;
	QString firstLine_;
	QString extFileName_;
	QTimer autosaveTimer_;
	bool trashRequested_;
	bool changed_;

	static QHash< QPair<QString,QString>, NoteDialog* > dialogs;

signals:
	void trash();
	void save();

public slots:
	void done(int r);

private slots:
	void autosave();
	void trashClicked();
	void copyClicked();
	void textChanged();
	void on_printBtn_clicked();
	void on_saveBtn_clicked();
};

#endif // NOTEDIALOG_H
