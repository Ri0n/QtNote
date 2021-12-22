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

#ifndef NOTEMANAGERVIEW_H
#define NOTEMANAGERVIEW_H

#include <QTreeView>

namespace QtNote {

class NoteManagerView : public QTreeView {
    Q_OBJECT
public:
    explicit NoteManagerView(QWidget *parent = nullptr);

protected:
    // reimplemented
    void contextMenuEvent(QContextMenuEvent *e);

signals:

public slots:
    void removeSelected();
};

} // namespace QtNote

#endif // NOTEMANAGERVIEW_H
