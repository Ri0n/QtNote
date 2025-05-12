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

#ifndef PTFSTORAGE_H
#define PTFSTORAGE_H

#include "filestorage.h"

namespace QtNote {

class PTFStorage : public FileStorage {
    Q_OBJECT
    Q_DISABLE_COPY(PTFStorage)

    QIcon icon;

public:
    PTFStorage(QObject *parent = 0);
    bool                init();
    bool                isAccessible() const;
    const QString       systemName() const;
    const QString       name() const;
    QIcon               storageIcon() const;
    QIcon               noteIcon() const;
    QList<NoteListItem> noteListFromInfoList(const QFileInfoList &);
    Note                note(const QString &noteId);
    QString             saveNote(const QString &noteId, const QString &text, Note::Format Format = Note::PlainText);
    QList<Note::Format> availableFormats() const;
    QString             findStorageDir() const;
};

} // namespace QtNote

#endif // PTFSTORAGE_H
