/*
QtNote - Simple note-taking application
Copyright (C) 2010 Sergei Ilinykh

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

#include "filestorage.h"
#include "filestoragesettingswidget.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QUuid>

#include <ranges>

namespace QtNote {

FileStorage::FileStorage(QObject *parent) : NoteStorage(parent), _cacheValid(false) { }

QString FileStorage::createNote(const QString &text, Note::Format Format) { return saveNote(QString(), text, Format); }

void FileStorage::deleteNote(const QString &noteId)
{
    auto r = cache.find(noteId);
    if (r == cache.end()) {
        return;
    }
    bool deleted = false;
    for (auto const &ext : std::as_const(fileExt)) {
        if (notesDir.remove(QString("%1.%2").arg(noteId, ext))) {
            deleted = true;
        }
    }
    if (deleted) {
        NoteListItem item = r.value();
        cache.erase(r);
        emit noteRemoved(item);
    }
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
    connect(w, &FileStorageSettingsWidget::apply, this, [this, w]() {
        QString p = w->path();
        if (p.isEmpty()) { /* just to be sure all native file dialogs work the same way */
            return;
        }
        QFileInfo fi(p);
        if (!fi.isDir() || !fi.isWritable()) {
            return;
        }
        auto path = fi.absoluteFilePath();
        notesDir.setPath(path);
        QSettings().setValue(QString("storage.%1.path").arg(systemName()),
                             notesDir.absolutePath() == findStorageDir() ? QString() : path);
        cache.clear();
        _cacheValid = false;
        init();
        emit invalidated();
    });
    return w;
}

QString FileStorage::tooltip() { return QString("<b>%1:</b> %2").arg(tr("Storage path"), notesDir.absolutePath()); }

void FileStorage::ensureChachePopulated()
{
    if (_cacheValid) {
        return;
    }
    if (isAccessible()) {
        QStringList wcards;
        // could be done with ranges but see https://bugreports.qt.io/browse/QTBUG-120924
        for (auto const &ext : std::as_const(fileExt)) {
            wcards.append(QLatin1String("*.") + ext);
        }

        QFileInfoList files = notesDir.entryInfoList(wcards, QDir::Files, QDir::Time);
        auto          ret   = noteListFromInfoList(files);
        cache.clear();
        for (auto &n : ret) {
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
    std::sort(ret.begin(), ret.end(), noteListItemModifyComparer);
    // probably sort is unnecesary here if the only accessor is notemanager which also does sorting.
    return limit ? ret.mid(0, limit) : ret;
}

} // namespace QtNote
