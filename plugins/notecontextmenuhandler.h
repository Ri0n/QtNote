#ifndef NOTECONTEXTMENUHANDLER_H
#define NOTECONTEXTMENUHANDLER_H

#include <QObject>

class QMenu;
class QContextMenuEvent;
class QTextEdit;

namespace QtNote {

class NoteContextMenuHandler
{
public:
    virtual void populateNoteContextMenu(QTextEdit *nw, QContextMenuEvent *event, QMenu *menu) = 0;
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::NoteContextMenuHandler,
                    "com.rion-soft.QtNote.NobteContextMenuHandler/1.0")

#endif // NOTECONTEXTMENUHANDLER_H
