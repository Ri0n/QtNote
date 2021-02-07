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

#ifndef NOTESTORAGE_H
#define NOTESTORAGE_H

#include <QDateTime>
#include <QIcon>
#include <QObject> // just for compatibility with qt<4.6

#include "note.h"
#include "qtnote_export.h"

namespace QtNote {

class NoteStorage;
class NoteFinder;

struct NoteListItem {
    NoteListItem(const QString &id_, const QString &storageId_, const QString &title_, const QDateTime &lastModify_) :
        id(id_), storageId(storageId_), title(title_), lastModify(lastModify_)
    {
    }
    QString   id;
    QString   storageId;
    QString   title;
    QDateTime lastModify;
};

inline bool noteListItemModifyComparer(const NoteListItem &a, const NoteListItem &b)
{
    return a.lastModify > b.lastModify; // backward order
}

class QTNOTE_EXPORT NoteStorage : public QObject {
    Q_OBJECT
public:
    typedef QSharedPointer<NoteStorage> Ptr;

    using QObject::QObject;
    virtual bool          init()               = 0;
    virtual const QString systemName() const   = 0;
    virtual const QString name() const         = 0;
    virtual QIcon         storageIcon() const  = 0;
    virtual QIcon         noteIcon() const     = 0;
    virtual bool          isAccessible() const = 0;

    /* 0 - not limit */
    virtual QList<NoteListItem> noteList(int limit = 0) = 0;

    /* should return null note (d=0) if not is not found */
    virtual Note note(const QString &id) = 0;

    /* returns note id */
    virtual QString createNote(const QString &text) = 0;
    /* returns updated note id or the same */
    virtual QString saveNote(const QString &noteId, const QString &text) = 0;
    virtual void    deleteNote(const QString &noteId)                    = 0;
    virtual bool    isRichTextAllowed() const                            = 0;

    virtual QWidget *settingsWidget() { return 0; }
    virtual QString  tooltip() { return QString(); }

signals:
    void noteAdded(const NoteListItem &);
    void noteModified(const NoteListItem &);
    void noteRemoved(const NoteListItem &);
    void noteIdChanged(const NoteListItem &note, const QString &oldNoteId);
    void invalidated();
    void storageErorr(const QString &);
};

} // namespace QtNote

#endif // NOTESTORAGE_H
