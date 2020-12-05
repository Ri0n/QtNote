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

#include <QContextMenuEvent>
#include <QMenu>

#include "notemanager.h"
#include "notemanagerview.h"
#include "notesmodel.h"

namespace QtNote {

NoteManagerView::NoteManagerView(QWidget *parent) : QTreeView(parent) { }

void NoteManagerView::contextMenuEvent(QContextMenuEvent *e)
{
    QModelIndex index = currentIndex();
    if (index.isValid() && index.parent().isValid()) { // note
        e->accept();
        QMenu m;
        m.addAction(QIcon(":/icons/trash"), tr("Delete"), this, SLOT(removeSelected()));
        m.exec(QCursor::pos());
    }
}

void NoteManagerView::removeSelected()
{
    QModelIndexList indexes = selectedIndexes();
    foreach (QModelIndex index, indexes) {
        NoteStorage::Ptr storage = NoteManager::instance()->storage(index.data(NotesModel::StorageIdRole).toString());
        QString          noteId  = index.data(NotesModel::NoteIdRole).toString();
        if (storage && !noteId.isEmpty()) {
            storage->deleteNote(noteId);
        }
    }
}

} // namespace QtNote
