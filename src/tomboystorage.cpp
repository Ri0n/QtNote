/*
QtNote - Simple note-taking application
Copyright (C) 2010 Ili'nykh Sergey

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Contacts:
E-Mail: rion4ik@gmail.com XMPP: rion@jabber.ru
*/

#include "tomboystorage.h"

#include "tomboydata.h"
#include <QDir>
#include <QCoreApplication>
#include <QDesktopServices>
#ifdef Q_OS_WIN
#include <QSettings>
#endif

TomboyStorage::TomboyStorage(QObject *parent)
	: FileStorage(parent)
{
	fileExt = "note";
	QStringList tomboyDirs;
	QString orgName = QCoreApplication::organizationName();
	QString appName = QCoreApplication::applicationName();
	QCoreApplication::setOrganizationName("");
	QCoreApplication::setApplicationName("");

	QString dataLocation = QDir::cleanPath(
			QDesktopServices::storageLocation(QDesktopServices::DataLocation) );
#ifdef Q_OS_UNIX
	tomboyDirs<<((dataLocation.endsWith("/data")?dataLocation.left(dataLocation.length()-5) : dataLocation)+"/tomboy");
	tomboyDirs<<(QDir::home().path()+"/.tomboy");
#elif defined(Q_OS_MAC)
	tomboyDirs<<(dataLocation + "/Tomboy");
	tomboyDirs<<(QDir::homePath() + "/.config/tomboy/");
#elif defined(Q_OS_WIN)
	QSettings ini(QSettings::IniFormat, QSettings::UserScope, "", "");
	dataLocation = QFileInfo(ini.fileName()).absolutePath();
	tomboyDirs<<(dataLocation + "/Tomboy/notes");
	tomboyDirs<<(dataLocation + "/tomboy");
#endif
	foreach (notesDir, tomboyDirs) {
		if (QDir(notesDir).isReadable()) {
			//qDebug("found tomboy dir: %s", qPrintable(notesDir));
			break;
		}
	}
	QCoreApplication::setOrganizationName(orgName);
	QCoreApplication::setApplicationName(appName);
}

bool TomboyStorage::isAccessible() const
{
	return QDir(notesDir).isReadable();
}

const QString TomboyStorage::systemName() const
{
	return "tomboy";
}

const QString TomboyStorage::titleName() const
{
	return tr("Tomboy Storage");
}

QList<NoteListItem> TomboyStorage::noteList()
{
	QList<NoteListItem> ret = cache.values();
	if (ret.count() == 0) {
		QFileInfoList files = QDir(notesDir).entryInfoList(QStringList(QString("*.")
									+ fileExt), QDir::Files | QDir::NoDotAndDotDot);
		foreach (QFileInfo fi, files) {
			TomboyData note;
			if (note.fromFile(fi.canonicalFilePath())) {
				NoteListItem li(note.uid(), systemName(), note.title(),
								note.modifyTime());
				ret.append(li);
				cache.insert(note.uid(), li);
			}
		}
	}
	return ret;
}

Note TomboyStorage::get(const QString &id)
{
	QString fileName = QDir(notesDir).absoluteFilePath(
			QString("%1.%2").arg(id).arg(fileExt) );
	TomboyData *noteData = new TomboyData;
	noteData->fromFile(fileName);
	return Note(noteData);
}

void TomboyStorage::saveNote(const QString &noteId, const QString &text)
{
	TomboyData note;
	note.setText(text);
	if (note.saveToFile( QDir(notesDir).absoluteFilePath(
			QString("%1.%2").arg(noteId).arg(fileExt)) )) {
		cache.insert(note.uid(), NoteListItem(note.uid(), systemName(),
											  note.title(), note.modifyTime()));
	}
}

bool TomboyStorage::isRichTextAllowed() const
{
	return true;
}
