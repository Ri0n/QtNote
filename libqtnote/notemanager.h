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

namespace QtNote {

class NoteManager : public QObject
{
	Q_OBJECT
public:
	static NoteManager *instance();
	void registerStorage(NoteStorage::Ptr storage, quint16 priority = 0);
	bool loadAll();

	QList<NoteListItem> noteList(int count = -1) const;

	Note getNote(const QString &storageId, const QString &noteId);

	const QMap<QString, NoteStorage::Ptr> storages() const;

	inline const QMap<quint16, NoteStorage::Ptr> prioritizedStorages() const
	{ return _prioritizedStorages; }

	NoteStorage::Ptr storage(const QString &storageId) const;
	inline NoteStorage::Ptr defaultStorage() const
	{ return _prioritizedStorages.constBegin().value(); }

	void updatePriorities(const QStringList &storageCodes);

signals:
	void storageAdded(NoteStorage::Ptr);
	void storageRemoved(NoteStorage::Ptr);
	void storageChanged(NoteStorage::Ptr);

private slots:
	void storageChanged();

private:
	NoteManager(QObject *parent);

	static NoteManager *_instance;
	QMap<QString, NoteStorage::Ptr> _storages;
	QMap<quint16, NoteStorage::Ptr> _prioritizedStorages;
};

}

#endif // NOTEMANAGER_H
