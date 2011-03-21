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

#ifndef FILESTORAGE_H
#define FILESTORAGE_H

#include "notestorage.h"
#include <QHash>

class FileStorage : public NoteStorage
{
public:
	FileStorage(QObject *parent);
	void createNote(const QString &text);
	void deleteNote(const QString &noteId);
	virtual void putToCache(const NoteListItem &note);

protected:
	QString fileExt;
	QHash<QString, NoteListItem> cache;
	QString notesDir;
};

#endif // FILESTORAGE_H
