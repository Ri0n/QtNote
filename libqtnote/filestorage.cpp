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

#include "filestorage.h"
#include "filestoragesettingswidget.h"

namespace QtNote {

FileStorage::FileStorage(QObject *parent)
    : NoteStorage(parent)
{

}

QString FileStorage::createNote(const QString &text)
{
    QString uid = QUuid::createUuid().toString();
    uid = uid.mid(1, uid.length()-2);
    saveNote(uid, text);
    return uid;
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

void FileStorage::putToCache(const NoteListItem &note)
{
    bool isModify = cache.contains(note.id);
    cache.insert(note.id, note);
    if (isModify) {
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
    notesDir = w->path();
    QSettings().setValue(QString("storage.%1.path").arg(systemName()), notesDir == findStorageDir()? "" : notesDir);
    cache.clear();
    emit invalidated();
}

QList<NoteListItem> FileStorage::noteList(int limit)
{
    QList<NoteListItem> ret = cache.values();
    // if cache is empty or fs may have more notes
    if (ret.count() == 0 || (ret.count() == cacheLimit && (limit == 0 || limit > cacheLimit))) {
        QFileInfoList files = QDir(notesDir).entryInfoList(QStringList(QString("*.")
                                    + fileExt), QDir::Files, QDir::Time);
        ret = noteListFromInfoList(files.mid(0, limit? limit : -1));
        cache.clear();
        for (auto n: ret) {
            cache.insert(n.id, n);
        }
        cacheLimit = limit;
    }
    return ret;
}

} // namespace QtNote

#include "filestorage.moc"
