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
#include <QMap>
#include "notestorage.h"

struct StorageItem {
	StorageItem()
		: storage(0)
	{

	}
	StorageItem(NoteStorage *storage_, int priority_)
		: storage(storage_)
		, priority(priority_)
	{

	}
	NoteStorage *storage;
	int priority;
};

class NoteManager : public QObject
{
	Q_OBJECT
public:
	static NoteManager *instance();
	void registerStorage(NoteStorage *storage, int priority = 0);
	bool loadAll();
	QList<NoteListItem> noteList() const;
	Note getNote(const QString &storageId, const QString &noteId);
	const QMap<QString, StorageItem> storages() const;
	const QList<StorageItem> prioritizedStorages() const;
	NoteStorage * storage(const QString &storageId) const;
	NoteStorage *defaultStorage();
	//QStringList storageCodes() const;
	void updatePriorities(const QStringList &storageCodes);

private:
	NoteManager(QObject *parent);
	static bool prioritySorter(const StorageItem &a, const StorageItem &b);

	static NoteManager *instance_;
	QMap<QString, StorageItem>storages_;
	NoteStorage *defaultStorage_;
};

#endif // NOTEMANAGER_H
