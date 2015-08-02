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
#include <QMenu>
#include <QApplication>
#include <QDesktopServices>
#include <QTextBlock>
#include <QDebug>

#include "noteedit.h"

namespace QtNote {

NoteEdit::NoteEdit(QWidget *parent) :
	QTextEdit(parent)
{
}

void NoteEdit::addContextMenuHandler(NoteContextMenuHandler *handler)
{
    menuHandlers.append(dynamic_cast<QObject*>(handler));
}

void NoteEdit::setLinkHighlightEnabled(bool state)
{
    if (state) {
        setMouseTracking(true);
    } else {
        viewport()->setCursor(Qt::IBeamCursor);
        setMouseTracking(false);
    }
}

void NoteEdit::dropEvent(QDropEvent *e)
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

void NoteEdit::focusInEvent(QFocusEvent *e)
{
	emit focusReceived();
    QTextEdit::focusInEvent(e);
}

void NoteEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    QMutableListIterator<QPointer<QObject>> it(menuHandlers);
    while (it.hasNext()) {
        auto p = dynamic_cast<NoteContextMenuHandler*>(it.next().data());
        if (p) {
            p->populateNoteContextMenu(this, event, menu);
        } else {
            it.remove();
        }
    }

    menu->exec(event->globalPos());
    delete menu;
}

void NoteEdit::focusOutEvent(QFocusEvent *e)
{
    setLinkHighlightEnabled(false);
	emit focusLost();
	QTextEdit::focusOutEvent(e);
}

QString NoteEdit::unparsedAnchorAt(const QPoint &pos)
{
    auto cur = cursorForPosition(pos);
    bool startFound = false;
    bool endFound = false;
    auto blockText = cur.block().text();
    int startPos, endPos;
    startPos = endPos = cur.positionInBlock();
    if (!blockText.length() || blockText[startPos].isSpace()) {
        return QString();
    }
    while (!(startFound && endFound)) {
        if (!startFound) {
            if (!startPos || blockText[startPos - 1].isSpace()) {
                startFound = true;
            } else {
                startPos--;
            }
        }
        if (!endFound) {
            endPos++;
            if (endPos >= blockText.length() || blockText[endPos].isSpace()) {
                endFound  = true;
            }
        }
    }
    QString matched = blockText.mid(startPos, endPos - startPos);
    if (matched.indexOf(QLatin1String("://")) > 0 || matched.startsWith("www.")) {
        return matched;
    }
    return QString();
}

void NoteEdit::mousePressEvent(QMouseEvent *e)
{
    QString url;
    if (e->modifiers() & Qt::ControlModifier && e->button() == Qt::LeftButton && !(url = unparsedAnchorAt(e->pos())).isEmpty()) {
        e->accept();
        QDesktopServices::openUrl(QUrl::fromUserInput(url));
        return;
    }
    QTextEdit::mousePressEvent(e);
}

void NoteEdit::mouseMoveEvent(QMouseEvent *e)
{
    QString url;
    if (!(url = unparsedAnchorAt(e->pos())).isEmpty()) {
        viewport()->setCursor(Qt::PointingHandCursor);
    } else {
        viewport()->setCursor(Qt::IBeamCursor);
    }
    QTextEdit::mouseMoveEvent(e);
}

void NoteEdit::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Control) {
        setLinkHighlightEnabled(true);
    }
    QTextEdit::keyPressEvent(e);
}

void NoteEdit::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Control) {
        setLinkHighlightEnabled(false);
    }
    QTextEdit::keyPressEvent(e);
}

} // namespace QtNote
