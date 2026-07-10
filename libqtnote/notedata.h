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

#ifndef NOTEDATA_H
#define NOTEDATA_H

#include <QSharedData>
#include <QString>
#include <QStringList>

#include "note.h"

namespace QtNote {

class NoteStorage;

class QTNOTE_EXPORT NoteData : public QSharedData {
public:
    static constexpr int TitleLength = 256;

    NoteData(NoteStorage *storage) : storage_(storage) { }
    virtual ~NoteData() = default;

    virtual QString storageId() const = 0;
    virtual bool    load()            = 0; // load `text_` field.
    virtual void    remove()          = 0;

    void setId(const QString &newId) { id_ = newId; }
    void unload()
    {
        text_   = {}; // should reset capacity
        loaded_ = false;
    }
    void setText(const QString &text, Note::Format format)
    {
        loaded_ = true;
        text_   = text;
        format_ = format;
        tags_   = tagsFromText(text_);
    }

    QStringList tags() const;

    static QStringList tagsFromLine(const QString &line);
    static QStringList tagsFromText(const QString &text);

protected:
    void setTags(const QStringList &tags);

    friend class Note;
    friend class NoteStorage;
    NoteStorage *storage_ = nullptr;
    bool         loaded_ { false };
    Note::Format format_; // format of the text_ field
    QString      id_;     // this can match with title sometimes
    QString      title_;
    QString      text_;
    QStringList  tags_;
    QDateTime    lastChange_;
};

} // namespace QtNote

#endif // NOTEDATA_H
