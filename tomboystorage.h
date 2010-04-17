#ifndef TOMBOYSTORAGE_H
#define TOMBOYSTORAGE_H

#include <QMap>
#include "notestorage.h"

class TomboyStorage : public NoteStorage
{
	Q_OBJECT
	Q_DISABLE_COPY(TomboyStorage)
public:
	TomboyStorage(QObject *parent);
	bool isAccessible() const;
	const QString systemName() const;
	QList<NoteListItem> noteList() const;
	Note* get(const QString &id);
	void createNote(const QString &text);
	void saveNote(const QString &noteId, const QString &text);
	void deleteNote(const QString &text);

private:
	QString notesDir;
	QMap<QString, Note*> notes;
};

#endif // TOMBOYSTORAGE_H
