#include "tomboystorage.h"

#include "tomboynote.h"
#include <QDir>

TomboyStorage::TomboyStorage(QObject *parent)
	: NoteStorage(parent)
{
	notesDir = QDir::home().path()+"/.tomboy";
}

bool TomboyStorage::isAccessible() const
{
	return QDir(notesDir).isReadable();
}

const QString TomboyStorage::systemName() const
{
	return "tomboy";
}

QList<NoteListItem> TomboyStorage::noteList() const
{
	QList<NoteListItem> ret;
	QFileInfoList files = QDir(notesDir).entryInfoList(QStringList("*.note"),
			  QDir::Files | QDir::NoDotAndDotDot);
	foreach (QFileInfo fi, files) {
		TomboyNote note(fi.canonicalFilePath());
		ret.append(NoteListItem(note.uid(), systemName(), note.title(), note.modifyTime()));
	}
	return ret;
}

Note* TomboyStorage::load(const QString &id) const
{
	return 0;
}
