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

// #include "note.h"
#include "notedata.h"
#include "notemanager.h"

#include <QString>
#include <QStringList>

namespace QtNote {

Note::Note() { }

Note::~Note() { }

Note::Note(NoteData *data) : d(QExplicitlySharedDataPointer<NoteData>(data)) { }

Note::Note(const Note &note) : d(note.d) { }

Note::Note(Note &&note) : d(std::move(note.d)) { }

Note &Note::operator=(const Note &note)
{
    d = note.d;
    return *this;
}

bool Note::isNull() const { return !d; }

bool Note::isEmpty() const { return isNull() || (d->title_.isEmpty() && d->text_.isEmpty()); }

bool Note::isLoaded() const { return !isNull() && d->loaded_; }

bool Note::save()
{
    auto storage = NoteManager::instance()->storage(d->storageId());
    if (storage) {
        return storage->saveNote(*this);
    }
    qWarning("failed to save note. storage not found");
    return false;
}

bool Note::load()
{
    auto storage = this->storage();
    return storage && storage->loadNote(*this);
}

void Note::remove()
{
    auto storage = this->storage();
    if (storage)
        storage->removeNote(id());
}

void Note::setTitle(const QString &title) { d->title_ = title; }

void Note::setText(const QString &text, Format format) { d->setText(text, format); }

void Note::setId(const QString &id) { d->setId(id); }

void Note::setFormat(Format format) { d->format_ = format; }

void Note::setTags(const QStringList &tags) { d->setTags(tags); }

void Note::unload() { d->unload(); }

void Note::setLastChangeUTC(const QDateTime &lastChange) { d->lastChange_ = lastChange; }

void Note::setBackendValue(const QString &key, const QVariant &value) { d->setBackendValue(key, value); }

NoteStorage *Note::storage() const { return d->storage_; }

QString Note::storageId() const { return d->storageId(); }

QString Note::id() const { return d->id_; }

QString Note::text() const { return d->text_; }

QString Note::title() const { return d->title_; }

QStringList Note::tags() const { return d->tags(); }

NoteData *Note::data() const { return d.data(); }

Note::Format Note::format() const { return d->format_; }

QDateTime Note::lastChangeUTC() const { return d->lastChange_; }

QVariant Note::backendValue(const QString &key) const { return d->backendValue(key); }

bool Note::isUpdated() const
{
    if (isNull() || id().isEmpty() || !storage()) {
        return true;
    }

    auto freshNote = storage()->note(id());
    return freshNote.isNull() || freshNote.lastChangeUTC() <= lastChangeUTC();
}

} // namespace QtNote
