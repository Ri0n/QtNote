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

#ifndef TOMBOYSTORAGE_H
#define TOMBOYSTORAGE_H

#include <QMap>
#include "notestorage.h"

class TomboyStorage : public NoteStorage
{
	Q_OBJECT
	Q_DISABLE_COPY(TomboyStorage)
public:
	TomboyStorage(QObject *parent);
	bool isAccessible() const;
	const QString systemName() const;
	QList<NoteListItem> noteList();
	Note* get(const QString &id);
	void createNote(const QString &text);
	void saveNote(const QString &noteId, const QString &text);
	void deleteNote(const QString &text);

private:
	QString notesDir;
	QMap<QString, Note*> notes;
};

#endif // TOMBOYSTORAGE_H
