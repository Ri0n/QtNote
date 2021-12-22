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

#include <QKeyEvent>

#include "qtnote.h"
#include "shortcutedit.h"
#include "shortcutsmanager.h"

namespace QtNote {

ShortcutEdit::ShortcutEdit(Main *qtnote, const QString &option, QWidget *parent) :
    QLineEdit(parent), qtnote(qtnote), option(option)
{
}

void ShortcutEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
        setText("");
        _seq = QKeySequence();
        setModified(true);
        return;
    }
    QString grab;
    int     modifiers = event->modifiers();
    if (modifiers & Qt::ControlModifier) {
        grab.append("Ctrl+");
    }
    if (modifiers & Qt::ShiftModifier) {
        grab.append("Shift+");
    }
    if (modifiers & Qt::AltModifier) {
        grab.append("Alt+");
    }
    if (modifiers & Qt::MetaModifier) {
        grab.append("Meta+");
    }
    QString      key = QKeySequence(event->key()).toString();
    QKeySequence seq(grab + key);
    bool         mod = seq != _seq;
    setSequence(QKeySequence(grab + key));
    setModified(mod);
}

void ShortcutEdit::focusInEvent(QFocusEvent *ev)
{
    qtnote->shortcutsManager()->setShortcutEnable(option, false);
    QLineEdit::focusInEvent(ev);
}

void ShortcutEdit::focusOutEvent(QFocusEvent *ev)
{
    qtnote->shortcutsManager()->setShortcutEnable(option, true);
    QLineEdit::focusOutEvent(ev);
}

}
