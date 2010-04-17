#include "qtnoteptfstorage.h"
#include <QDesktopServices>
#include <QDir>

QtNotePTFStorage::QtNotePTFStorage(QObject *parent)
	: NoteStorage(parent)
{

}

bool QtNotePTFStorage::isAccessible() const
{
	return false;
//	return QDir(
//		QDesktopServices::StandardLocation(QDesktopServices::DataLocation) +
//		"/" + systemName()
//		).isReadable();
}

const QString QtNotePTFStorage::systemName() const
{
	return "ptf";
}

QList<NoteListItem> QtNotePTFStorage::noteList() const
{
	return QList<NoteListItem>();
}

Note* QtNotePTFStorage::get(const QString &id)
{
	Q_UNUSED(id);
	return 0;
}

void QtNotePTFStorage::createNote(const QString &text)
{

}

void QtNotePTFStorage::saveNote(const QString &noteId, const QString &text)
{

}

void QtNotePTFStorage::deleteNote(const QString &noteId)
{

}
