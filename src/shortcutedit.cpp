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

#include <QKeyEvent>

#include "shortcutedit.h"

ShortcutEdit::ShortcutEdit(QWidget *parent) :
	QLineEdit(parent)
{
}

void ShortcutEdit::keyPressEvent(QKeyEvent *event)
{
	QString grab;
	int modifiers = event->modifiers();
	if(modifiers & Qt::ControlModifier) {
		grab.append("Ctrl+");
	}
	if(modifiers & Qt::ShiftModifier) {
		grab.append("Shift+");
	}
	if(modifiers & Qt::AltModifier) {
		grab.append("Alt+");
	}
	if(modifiers & Qt::MetaModifier) {
		grab.append("Meta+");
	}
	QString key =  QKeySequence(event->key()).toString();
	qDebug("SEQ: %s", qPrintable(grab + key));
	setSequence(QKeySequence(grab + key));
}
