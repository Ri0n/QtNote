#include "tomboystorage.h"

#include "tomboynote.h"
#include <QDir>
#include <QUuid>

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

Note* TomboyStorage::get(const QString &id)
{
	QString fileName = QDir(notesDir).absoluteFilePath(
			QString("%1.note").arg(id) );
	qDebug("loading: %s:%s\nfilename: %s", qPrintable(systemName()),
		   qPrintable(id), qPrintable(fileName));
	TomboyNote *note = new TomboyNote(this);
	if (note->fromFile(fileName)) {
		notes[id] = note;
		return note;
	}
	delete note;
	return 0;
}

void TomboyStorage::createNote(const QString &text)
{
	QString uid = QUuid::createUuid ().toString();
	uid.mid(1, uid.length()-2);
	saveNote(uid, text);
}

void TomboyStorage::saveNote(const QString &noteId, const QString &text)
{
	TomboyNote *note = new TomboyNote(this);
	note->setText(text);
	note->saveToFile( QDir(notesDir).absoluteFilePath(
			QString("%1.note").arg(noteId)) );
}

void TomboyStorage::deleteNote(const QString &noteId)
{
	QFile::remove( QDir(notesDir).absoluteFilePath(
			QString("%1.note").arg(noteId)) );
}
