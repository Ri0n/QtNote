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

#include <QDesktopServices>
#include <QDir>
#include <QApplication>
#include <QStyle>
#include <QSettings>

#ifdef Q_OS_WIN
#include <QLibrary>
#include <ShlObj.h>
#endif // Q_OS_WIN

#include "ptfstorage.h"
#include "ptfdata.h"
#include "ptfstoragesettingswidget.h"
#include "utils.h"

PTFStorage::PTFStorage(QObject *parent)
	: FileStorage(parent)
{
	fileExt = "txt";
	QSettings s;
	notesDir = s.value("storage.ptf.path").toString();
	if (notesDir.isEmpty()) {
		notesDir = Utils::localDataDir() + QLatin1Char('/') + systemName();
	}
	initNotesDir();
}

void PTFStorage::initNotesDir()
{
	QDir d(notesDir);
	if (!d.exists()) {
		if (!QDir::root().mkpath(notesDir)) {
			qWarning("can't create storage dir: %s", qPrintable(notesDir));
		}
	}
}

bool PTFStorage::isAccessible() const
{
	return QDir(notesDir).isReadable();
}

const QString PTFStorage::systemName() const
{
	return "ptf";
}

const QString PTFStorage::titleName() const
{
	return tr("Plain Text Storage");
}

QIcon PTFStorage::storageIcon() const
{
	return QIcon(":/icons/trayicon");
}

QIcon PTFStorage::noteIcon() const
{
	return QApplication::style()->standardIcon(
				QStyle::SP_MessageBoxInformation);
}

QList<NoteListItem> PTFStorage::noteList()
{
	QList<NoteListItem> ret = cache.values();
	if (ret.count() == 0) {
		QFileInfoList files = QDir(notesDir).entryInfoList(QStringList(QString("*.")
									+ fileExt), QDir::Files | QDir::NoDotAndDotDot);
		foreach (QFileInfo fi, files) {
			PTFData note;
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

Note PTFStorage::get(const QString &noteId)
{
	QString fileName = QDir(notesDir).absoluteFilePath(
			QString("%1.%2").arg(noteId).arg(fileExt) );
	PTFData *noteData = new PTFData;
	noteData->fromFile(fileName);
	return Note(noteData);
}

void PTFStorage::saveNote(const QString &noteId, const QString & text)
{
	PTFData note;
	note.setText(text);
	if (note.saveToFile( QDir(notesDir).absoluteFilePath(
			QString("%1.%2").arg(noteId).arg(fileExt)) )) {
		NoteListItem item(note.uid(), systemName(), note.title(), note.modifyTime());
		putToCache(item);
	}
}

bool PTFStorage::isRichTextAllowed() const
{
	return false;
}

QWidget *PTFStorage::settingsWidget() const
{
	PTFStorageSettingsWidget *w = new PTFStorageSettingsWidget(notesDir);
	connect(w, SIGNAL(apply()), SLOT(settingsApplied()));
	return w;
}

void PTFStorage::settingsApplied()
{
	PTFStorageSettingsWidget *w = reinterpret_cast<PTFStorageSettingsWidget *>(sender());
	notesDir = w->path();
	QSettings().setValue(QLatin1String("storage.ptf.path"), notesDir);
	initNotesDir();
	emit invalidated();
}
