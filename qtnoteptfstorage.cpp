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

Note* QtNotePTFStorage::load(const QString &id) const
{
	return 0;
}