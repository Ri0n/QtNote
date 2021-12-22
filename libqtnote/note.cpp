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

#include "note.h"
#include "notedata.h"
#include <QString>

namespace QtNote {

Note::Note() { }

Note::Note(NoteData *data) : d(QExplicitlySharedDataPointer<NoteData>(data)) { }

bool Note::isNull() { return !d; }

void Note::toTrash() { d->remove(); }

QString Note::text() const { return d->text(); }

QString Note::title() const { return d->title(); }

NoteData *Note::data() const { return d.data(); }

qint64 Note::lastChangeElapsed() const { return d->lastChangeElapsed(); }

} // namespace QtNote
