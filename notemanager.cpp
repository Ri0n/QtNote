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

#include "notemanager.h"
#include <QApplication>

NoteManager::NoteManager(QObject *parent)
	: QObject(parent)
	, defaultStorage_(0)
{

}

void NoteManager::registerStorage(NoteStorage *storage, bool isDefault)
{
	storages_.append(storage);
	if (isDefault) {
		defaultStorage_ = storage;
	}
}

bool NoteManager::loadAll()
{
	if (storages_.count() == 0) {
		return false;
	}
	if (!defaultStorage_) {
		defaultStorage_ = storages_[0];
	}
	return true;
}

QList<NoteListItem> NoteManager::noteList() const
{
	QList<NoteListItem> ret;
	foreach (NoteStorage *storage, storages_) {
		ret += storage->noteList();
	}
	qSort(ret.begin(), ret.end(), noteListItemModifyComparer);
	return ret;
}

Note NoteManager::getNote(const QString &storageId, const QString &noteId)
{
	NoteStorage *s = storage(storageId);
	if (s) {
		return s->get(noteId);
	}
	return 0;
}

const QList<NoteStorage *> NoteManager::storages() const
{
	return storages_;
}

NoteStorage * NoteManager::storage(const QString &storageId) const
{
	foreach (NoteStorage *storage, storages_) {
		if (storage->systemName() == storageId) {
			return storage;
		}
	}
	return 0;
}

NoteStorage *NoteManager::defaultStorage() const
{
	return defaultStorage_;
}

NoteManager *NoteManager::instance()
{
	if (!NoteManager::instance_) {
		NoteManager::instance_ = new NoteManager(QApplication::instance());
	}
	return NoteManager::instance_;
}

NoteManager* NoteManager::instance_ = NULL;
