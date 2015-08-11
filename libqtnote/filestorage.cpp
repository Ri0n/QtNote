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

#include <QUuid>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QDebug>

#include "filestorage.h"
#include "filestoragesettingswidget.h"
#include "filenotedata.h"
#include "filenameprovider.h"

namespace QtNote {

FileStorage::FileStorage(QObject *parent) :
    NoteStorage(parent),
    _cacheValid(false),
    nameProvider(0)
{

}

QString FileStorage::createNote(const QString &text)
{
    return saveNote(QString(), text);
}

void FileStorage::deleteNote(const QString &noteId)
{
    QHash<QString, NoteListItem>::const_iterator r = cache.find(noteId);
    if (r != cache.end()) {
        if (QFile::remove( QDir(notesDir).absoluteFilePath(
                QString("%1.%2").arg(noteId).arg(fileExt)) )) {
            NoteListItem item = r.value();
            cache.remove(r.key());
            emit noteRemoved(item);
        }
    }
}

QString FileStorage::saveNoteToFile(FileNoteData &note, const QString &text, const QString &noteId)
{
    QString newNoteId = noteId;
    QString fileName;

    if (noteId.isEmpty()) {
        fileName = nameProvider->newName(note, newNoteId);
    } else {
        fileName = nameProvider->updateName(note, newNoteId);
    }

    note.setText(text);
    if (note.saveToFile(fileName)) {
        if (!noteId.isEmpty() && noteId != newNoteId) {
            // TODO get old filename and remove old file
        }

        QString uid = nameProvider->uidForFileName(fileName);
        NoteListItem item(uid, systemName(), note.title(), note.modifyTime());
        putToCache(item, noteId); // noteId is old one
        return newNoteId;
    }
    handleFSError();
    return QString::null;
}

void FileStorage::handleFSError()
{
    cache.clear();
    _cacheValid = false;
    emit storageErorr(tr("File system error for storage \"%1\". Please check your settings.").arg(name()));
    emit invalidated();
}

void FileStorage::putToCache(const NoteListItem &note, const QString &oldNoteId)
{
    bool isModify;
    bool idChange = false;

    ensureChachePopulated();

    if (!oldNoteId.isEmpty() && oldNoteId != note.id) {
        isModify = cache.contains(oldNoteId);
        idChange = true;
    } else {
        isModify = cache.contains(note.id);
    }

    cache.insert(note.id, note);
    if (isModify) {
        if (idChange) {
            cache.remove(oldNoteId);
            emit noteIdChanged(note, oldNoteId);
        }
        emit noteModified(note);
    } else {
        emit noteAdded(note);
    }
}

QWidget *FileStorage::settingsWidget()
{
    FileStorageSettingsWidget *w = new FileStorageSettingsWidget(
                QSettings().value(QString("storage.%1.path").arg(systemName())).toString(), this);
    connect(w, SIGNAL(apply()), SLOT(settingsApplied()));
    return w;
}

QString FileStorage::tooltip()
{
    return QString("<b>%1:</b> %2").arg(tr("Storage path"), notesDir);
}

void FileStorage::settingsApplied()
{
    FileStorageSettingsWidget *w = qobject_cast<FileStorageSettingsWidget *>(sender());
    QString p = w->path();
    if (p.isEmpty()) { /* just to be sure all native file dialogs work the same way */
        return;
    }
    QFileInfo fi(p);
    if (!fi.isDir() || !fi.isWritable()) {
        return;
    }
    notesDir = fi.absoluteFilePath();
    QSettings().setValue(QString("storage.%1.path").arg(systemName()), notesDir == findStorageDir()? "" : notesDir);
    cache.clear();
    _cacheValid = false;
    emit invalidated();
}

void FileStorage::ensureChachePopulated()
{
    if (_cacheValid) {
        return;
    }
    QDir d(notesDir);
    if (d.isReadable()) {
        QFileInfoList files = d.entryInfoList(QStringList(QString("*.")
                                    + fileExt), QDir::Files, QDir::Time);
        auto ret = noteListFromInfoList(files);
        cache.clear();
        for (auto n: ret) {
            cache.insert(n.id, n);
        }
        _cacheValid = true;
    } else {
        handleFSError();
    }
}

QList<NoteListItem> FileStorage::noteList(int limit)
{
    ensureChachePopulated();
    QList<NoteListItem> ret = cache.values();
    qSort(ret.begin(), ret.end(), noteListItemModifyComparer);
    // probably sort is unnecesary here if the only accessor is notemanager which also does sorting.
    return limit ? ret.mid(0, limit) : ret;
}

} // namespace QtNote
