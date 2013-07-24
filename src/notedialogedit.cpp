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

#include <QDropEvent>
#include <QUrl>
#include <QTextCursor>
#include <QMimeData>

#include "notedialogedit.h"


NoteDialogEdit::NoteDialogEdit(QWidget *parent) :
    QTextEdit(parent)
{
}

void NoteDialogEdit::dropEvent(QDropEvent *e)
{
	bool useDefault = true;
	foreach (const QUrl &url, e->mimeData()->urls()) {
		QString fileName = url.toLocalFile();
		if (fileName.isEmpty()) {
			continue;
		}
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly)) {
			continue;
		}
		useDefault = false;
		QByteArray data = file.readAll();
		file.close();
		QString str = QString::fromLocal8Bit(data.constData(), data.size());
		textCursor().insertText(str);
	}
	if (useDefault) {
		QTextEdit::dropEvent(e);
	} else {
		e->accept();
	}
}
