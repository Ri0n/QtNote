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

#ifndef FILESTORAGE_H
#define FILESTORAGE_H

#include <QFileInfoList>
#include <QHash>

#include "qtnote_export.h"
#include "notestorage.h"

namespace QtNote {

class QTNOTE_EXPORT FileStorage : public NoteStorage
{
    Q_OBJECT
public:
    FileStorage(QObject *parent);
    QString createNote(const QString &text);
    void deleteNote(const QString &noteId);
    virtual void putToCache(const NoteListItem &note);
    virtual QString findStorageDir() const = 0;
    QWidget *settingsWidget();
    QString tooltip();
    QList<NoteListItem> noteList(int limit = 0);
    virtual QList<NoteListItem> noteListFromInfoList(const QFileInfoList &) = 0;

protected slots:
    void settingsApplied();

protected:
    QString fileExt;
    QHash<QString, NoteListItem> cache;
    int cacheLimit; /* last limit passed to noteList() */
    QString notesDir;
};

}

#endif // FILESTORAGE_H
