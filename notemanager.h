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
	Note* getNote(const QString &storageId, const QString &noteId);
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
