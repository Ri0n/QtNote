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

#ifndef NOTEMANAGER_H
#define NOTEMANAGER_H

#include <QObject>
#include "notestorage.h"

class NoteManager : public QObject
{
	Q_OBJECT
public:
	static NoteManager *instance();
	void registerStorage(NoteStorage *storage, bool isDefault = false);
	bool loadAll();
	QList<NoteListItem> noteList() const;
	Note getNote(const QString &storageId, const QString &noteId);
	const QList<NoteStorage *> storages() const;
	NoteStorage * storage(const QString &storageId) const;
	NoteStorage *defaultStorage() const;

private:
	NoteManager(QObject *parent);

	static NoteManager *instance_;
	QList<NoteStorage *>storages_;
	NoteStorage *defaultStorage_;
};

#endif // NOTEMANAGER_H
