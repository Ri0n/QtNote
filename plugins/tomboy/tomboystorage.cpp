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

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QSettings>

#include "tomboydata.h"
#include "tomboystorage.h"
#include "uuidfilenameprovider.h"

namespace QtNote {

TomboyStorage::TomboyStorage(QObject *parent) : FileStorage(parent)
{
    fileExt = "note";
}

bool TomboyStorage::init()
{
    QSettings s;
    notesDir = s.value("storage.tomboy.path").toString();
    if (notesDir.isEmpty() || !QDir(notesDir).isReadable()) {
        notesDir = findStorageDir();
    }
    if (!nameProvider) {
        nameProvider = new UuidFileNameProvider(notesDir, fileExt);
    } else {
        nameProvider->setPath(notesDir);
    }
    return isAccessible();
}

bool TomboyStorage::isAccessible() const { return !notesDir.isEmpty() && QDir(notesDir).isReadable(); }

const QString TomboyStorage::systemName() const { return "tomboy"; }

const QString TomboyStorage::name() const { return tr("Tomboy Storage"); }

QIcon TomboyStorage::storageIcon() const { return QIcon(":/icons/tomboy"); }

QIcon TomboyStorage::noteIcon() const { return QIcon(":/icons/tomboynote"); }

QList<NoteListItem> TomboyStorage::noteListFromInfoList(const QFileInfoList &files)
{
    QList<NoteListItem> ret;
    foreach (QFileInfo fi, files) {
        TomboyData note;
        if (note.fromFile(fi.canonicalFilePath())) {
            NoteListItem li(nameProvider->uidForFileName(fi.fileName()), systemName(), note.title(), note.modifyTime());
            ret.append(li);
        }
    }
    return ret;
}

Note TomboyStorage::note(const QString &id)
{
    if (!id.isEmpty()) {
        QString   fileName = QDir(notesDir).absoluteFilePath(QString("%1.%2").arg(id, fileExt));
        QFileInfo fi(fileName);
        if (fi.isWritable()) {
            TomboyData *noteData = new TomboyData;
            noteData->fromFile(fileName);
            return Note(noteData);
        }
        handleFSError();
    }
    return Note();
}

QString TomboyStorage::saveNote(const QString &noteId, const QString &text)
{
    TomboyData note;
    return saveNoteToFile(note, text, noteId);
}

bool TomboyStorage::isRichTextAllowed() const { return true; }

QString TomboyStorage::findStorageDir() const
{
    QStringList tomboyDirs;

    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
#ifdef Q_OS_UNIX
    tomboyDirs << (dataLocation + QLatin1String("/tomboy"));
    tomboyDirs << (QDir::home().path() + "/.tomboy");
#elif defined(Q_OS_MAC)
    tomboyDirs << (dataLocation + "/Tomboy");
    tomboyDirs << (QDir::homePath() + "/.config/tomboy/");
#elif defined(Q_OS_WIN)
    tomboyDirs << (dataLocation + "/Tomboy/notes");
    tomboyDirs << (dataLocation + "/tomboy");
#endif
    foreach (const QString &d, tomboyDirs) {
        QFileInfo fi(d);
        if (fi.isDir() && fi.isWritable()) {
            return d;
        }
    }
    return QString();
}

} // namespace QtNote
