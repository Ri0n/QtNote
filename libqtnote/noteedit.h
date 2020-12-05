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

#ifndef NOTEEDIT_H
#define NOTEEDIT_H

#include <QPointer>
#include <QTextBlock>
#include <QTextEdit>

#include "notecontextmenuhandler.h"
#include "qtnote_export.h"

class QDropEvent;

namespace QtNote {

class QTNOTE_EXPORT NoteEdit : public QTextEdit {
public:
    struct HoveredLinkPosition {
        QTextBlock block;
        int        pos;
        int        length;
    };

private:
    Q_OBJECT

    QList<QPointer<QObject>> menuHandlers;
    HoveredLinkPosition      hlp;

public:
    explicit NoteEdit(QWidget *parent = nullptr);
    virtual void                      addContextMenuHandler(NoteContextMenuHandler *handler);
    inline const HoveredLinkPosition &hoveredLinkPosition() const { return hlp; }

protected:
    void dropEvent(QDropEvent *e);
    void focusOutEvent(QFocusEvent *event);
    void focusInEvent(QFocusEvent *);
    void contextMenuEvent(QContextMenuEvent *event);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);

private:
    QString unparsedAnchorAt(const QPoint &pos);
    void    setLinkHighlightEnabled(bool state);

signals:
    void focusLost();
    void focusReceived();
    void linkHovered();
    void linkUnhovered();
};

} // namespace QtNote

#endif // NOTEEDIT_H
