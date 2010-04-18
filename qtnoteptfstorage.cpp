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

#include "qtnoteptfstorage.h"
#include <QDesktopServices>
#include <QDir>

QtNotePTFStorage::QtNotePTFStorage(QObject *parent)
	: NoteStorage(parent)
{

}

bool QtNotePTFStorage::isAccessible() const
{
	return false;
//	return QDir(
//		QDesktopServices::StandardLocation(QDesktopServices::DataLocation) +
//		"/" + systemName()
//		).isReadable();
}

const QString QtNotePTFStorage::systemName() const
{
	return "ptf";
}

QList<NoteListItem> QtNotePTFStorage::noteList()
{
	return QList<NoteListItem>();
}

Note* QtNotePTFStorage::get(const QString &id)
{
	Q_UNUSED(id);
	return 0;
}

void QtNotePTFStorage::createNote(const QString &text)
{
	Q_UNUSED(text)
}

void QtNotePTFStorage::saveNote(const QString &noteId, const QString &text)
{
	Q_UNUSED(noteId)
	Q_UNUSED(text)
}

void QtNotePTFStorage::deleteNote(const QString &noteId)
{
	Q_UNUSED(noteId)
}
