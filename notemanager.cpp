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
	return ret;
}

Note* NoteManager::getNote(const QString &storageId, const QString &noteId)
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