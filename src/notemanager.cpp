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
#include <QStringList>

NoteManager::NoteManager(QObject *parent)
	: QObject(parent)
	, defaultStorage_(0)
{

}

void NoteManager::registerStorage(NoteStorage *storage, int priority)
{
	//qDebug("%s:%d", qPrintable(storage->systemName()), priority);
	static int maxPriority = -1;
	storages_.insert(storage->systemName(), StorageItem(storage, priority));
	if (priority > maxPriority) {
		defaultStorage_ = storage;
		maxPriority = priority;
	}
}

bool NoteManager::loadAll()
{
	// mostly stubbed method for future use.
	if (storages_.count() == 0) {
		return false;
	}
	return true;
}

QList<NoteListItem> NoteManager::noteList(int count) const
{
	QList<NoteListItem> ret;
	foreach (StorageItem si, prioritizedStorages()) {
		QList<NoteListItem> items = si.storage->noteList();
		qSort(items.begin(), items.end(), noteListItemModifyComparer);
		ret += items;
	}
	return ret.mid(0, count);
}

Note NoteManager::getNote(const QString &storageId, const QString &noteId)
{
	NoteStorage *s = storage(storageId);
	if (s) {
		return s->get(noteId);
	}
	return 0;
}

const QMap<QString, StorageItem> NoteManager::storages() const
{
	return storages_;
}

bool NoteManager::prioritySorter(const StorageItem &a, const StorageItem &b)
{
	return a.priority > b.priority;
}

const QList<StorageItem> NoteManager::prioritizedStorages() const
{
	QList<StorageItem> items = NoteManager::instance()->storages().values();
	qSort(items.begin(), items.end(), prioritySorter);
	return items;
}

NoteStorage * NoteManager::storage(const QString &storageId) const
{
	// returns 0 if doesn't exists (see default contstructor StorageItem)
	return storages_.value(storageId).storage;
}

NoteStorage *NoteManager::defaultStorage()
{
	if (!defaultStorage_) {
		defaultStorage_ = prioritizedStorages()[0].storage;
	}
	//qDebug("default storage: %s", qPrintable(defaultStorage_->systemName()));
	return defaultStorage_;
}

//QStringList NoteManager::storageCodes() const
//{
//	return storages_.keys();
//}

void NoteManager::updatePriorities(const QStringList &storageCodes)
{
	defaultStorage_ = 0;
	for (int i=0, c=storageCodes.count(); i<c; i++) {
		QString code = storageCodes[c-i-1];
		if (storages_.contains(code)) {
			storages_[code].priority = i;
			//qDebug("changed to: %s:%d", qPrintable(storages_[code].storage->systemName()), i);
			defaultStorage_ = storages_[code].storage;
		}
	}
}

NoteManager *NoteManager::instance()
{
	if (!NoteManager::instance_) {
		NoteManager::instance_ = new NoteManager(QApplication::instance());
	}
	return NoteManager::instance_;
}

NoteManager* NoteManager::instance_ = NULL;
