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

#include "filestorage.h"
#include <QUuid>
#include <QFile>
#include <QDir>

FileStorage::FileStorage(QObject *parent)
	: NoteStorage(parent)
{

}

QString FileStorage::createNote(const QString &text)
{
	QString uid = QUuid::createUuid().toString();
	uid = uid.mid(1, uid.length()-2);
	saveNote(uid, text);
	return uid;
}

void FileStorage::deleteNote(const QString &noteId)
{
	QHash<QString, NoteListItem>::const_iterator r = cache.find(noteId);
	if (r != cache.end()) {
		if (QFile::remove( QDir(notesDir).absoluteFilePath(
				QString("%1.%2").arg(noteId).arg(fileExt)) )) {
			NoteListItem item = r.value();
			cache.remove(r.key());
			emit noteRemoved(item);
		}
	}
}

void FileStorage::putToCache(const NoteListItem &note)
{
	bool isModify = cache.contains(note.id);
	cache.insert(note.id, note);
	if (isModify) {
		emit noteModified(note);
	} else {
		emit noteAdded(note);
	}
}
