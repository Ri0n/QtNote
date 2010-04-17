#ifndef NOTESTORAGE_H
#define NOTESTORAGE_H

#include <QObject>
#include <QDateTime>
#include "note.h"

struct NoteListItem
{
	NoteListItem(const QString &id_, const QString &storageId_,
				 const QString &title_,const QDateTime &lastModify_)
		: id(id_)
		, storageId(storageId_)
		, title(title_)
		, lastModify(lastModify_) { }
	QString id;
	QString storageId;
	QString title;
	QDateTime lastModify;
};

class NoteStorage : public QObject
{
	Q_OBJECT
public:
	NoteStorage(QObject *parent);
	virtual const QString systemName() const = 0;
	virtual bool isAccessible() const = 0;
	virtual QList<NoteListItem> noteList() const = 0;
	virtual Note* get(const QString &id) = 0;
	virtual void createNote(const QString &text) = 0;
	virtual void saveNote(const QString &noteId, const QString &text) = 0;
	virtual void deleteNote(const QString &text) = 0;
};

#endif // NOTESTORAGE_H
