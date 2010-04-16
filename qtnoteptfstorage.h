#ifndef QTNOTEPTFSTORAGE_H
#define QTNOTEPTFSTORAGE_H

#include "notestorage.h"

class QtNotePTFStorage : public NoteStorage
{
	Q_OBJECT
	Q_DISABLE_COPY(QtNotePTFStorage)
public:
	QtNotePTFStorage(QObject *parent);
	bool isAccessible() const;
	const QString systemName() const;
	QList<NoteListItem> noteList() const;
	Note* load(const QString &id) const;
};

#endif // QTNOTEPTFSTORAGE_H
