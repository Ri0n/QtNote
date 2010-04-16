#ifndef TOMBOYSTORAGE_H
#define TOMBOYSTORAGE_H

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
	Note* load(const QString &id) const;

private:
	QString notesDir;
};

#endif // TOMBOYSTORAGE_H
