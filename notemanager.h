#ifndef NOTEMANAGER_H
#define NOTEMANAGER_H

#include <QObject>
#include "notestorage.h"

class NoteManager : public QObject
{
	Q_OBJECT
public:
	static NoteManager *instance();
	void registerStorage(NoteStorage *storage);
	bool loadAll();
	QList<NoteListItem> noteList() const;
	Note* loadNote(const QString &storageId, const QString &noteId);
	const QList<NoteStorage *> storages() const;

private:
	NoteManager(QObject *parent);

	static NoteManager *instance_;
	QList<NoteStorage *>storages_;
};

#endif // NOTEMANAGER_H
