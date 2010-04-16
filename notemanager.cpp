#include "notemanager.h"
#include <QApplication>

NoteManager::NoteManager(QObject *parent)
	: QObject(parent)
{

}


void NoteManager::registerStorage(NoteStorage *storage)
{
	storages_.append(storage);
}

bool NoteManager::loadAll()
{
	if (storages_.count() == 0) {
		return false;
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

Note* NoteManager::loadNote(const QString &storageId, const QString &noteId)
{
	foreach (NoteStorage *storage, storages_) {
		if (storage->systemName() == storageId) {
			return storage->load(noteId);
		}
	}
	return 0;
}

const QList<NoteStorage *> NoteManager::storages() const
{
	return storages_;
}

NoteManager *NoteManager::instance()
{
	if (!NoteManager::instance_) {
		NoteManager::instance_ = new NoteManager(QApplication::instance());
	}
	return NoteManager::instance_;
}

NoteManager* NoteManager::instance_ = NULL;