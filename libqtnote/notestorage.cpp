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

#include <QTimer>

#include "notestorage.h"

namespace QtNote {

bool noteListItemModifyComparer(const NoteListItem &a,
                                const NoteListItem &b) {
    return a.lastModify > b.lastModify; //backward order
}

NoteStorage::NoteStorage(QObject *parent)
    : QObject(parent)
{

}

NoteFinder *NoteStorage::search()
{
    return new NoteFinder(this);
}

void NoteFinder::start(const QString &text)
{
    auto nl = _storage->noteList();
    for (auto n : nl) {
        // text always returns plain text
        if (_storage->note(n.id).text().contains(text)) {
            emit found(n.id);
        }
    }

    emit completed();
    deleteLater();
}

void NoteFinder::abort()
{

}

} // namespace QtNote
