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

#ifndef NOTESTORAGE_H
#define NOTESTORAGE_H

#include <QDateTime>
#include "note.h"

struct NoteListItem
{
	NoteListItem(const QString &id_, const QString &storageId_,
				 const QString &title_,const QDateTime &lastModify_)
		: id(id_)
		, storageId(storageId_)
		, title(title_)
		, lastModify(lastModify_) { }
	QString id;
	QString storageId;
	QString title;
	QDateTime lastModify;
};

class NoteStorage : public QObject
{
	Q_OBJECT
public:
	NoteStorage(QObject *parent);
	virtual const QString systemName() const = 0;
	virtual bool isAccessible() const = 0;
	virtual QList<NoteListItem> noteList() = 0;
	virtual Note get(const QString &id) = 0;
	virtual void createNote(const QString &text) = 0;
	virtual void saveNote(const QString &noteId, const QString &text) = 0;
	virtual void deleteNote(const QString &text) = 0;
};

#endif // NOTESTORAGE_H
