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

#include <QApplication>
#include <QDesktopServices>
#include <QDropEvent>
#include <QFile>
#include <QMenu>
#include <QMimeData>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextLayout>
#include <QUrl>

#include "noteedit.h"

namespace QtNote {

NoteEdit::NoteEdit(QWidget *parent) : QTextEdit(parent) { }

void NoteEdit::addContextMenuHandler(NoteContextMenuHandler *handler)
{
    menuHandlers.append(dynamic_cast<QObject *>(handler));
}

void NoteEdit::setLinkHighlightEnabled(bool state)
{
    if (state) {
        setMouseTracking(true);
    } else {
        if (viewport()->cursor().shape() == Qt::PointingHandCursor) {
            viewport()->setCursor(Qt::IBeamCursor);
            emit linkUnhovered();
        }
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
        useDefault      = false;
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
    setLinkHighlightEnabled(unconditionalLinks || qApp->keyboardModifiers() & Qt::ControlModifier);
    emit focusReceived();
    QTextEdit::focusInEvent(e);
}

void NoteEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu                                  *menu = createStandardContextMenu();
    QMutableListIterator<QPointer<QObject>> it(menuHandlers);
    while (it.hasNext()) {
        auto p = dynamic_cast<NoteContextMenuHandler *>(it.next().data());
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

QString NoteEdit::locateLinkAt(const QPoint &pos)
{
    QTextCursor cursor = cursorForPosition(pos);
    if (cursor.isNull()) {
        return {};
    }
    auto cr = cursorRect(cursor);
    if (pos.y() < cr.top() || pos.y() >= cr.bottom()) {
        // qDebug() << "ignore by y position";
        return {};
    }
    // qDebug() << "mouse " << pos << " text cursor rect " << cr;
    if (pos.x() < cr.left()) {
        // Text cursor is on the right from mouse because. It was the closest to the mouse position.
        // But this way while we may still pointing at a link, the text cursor could move to the
        // next text fragment without any link. So we better take the previous position.
        // TODO: consider rtl text?
        cursor.movePosition(QTextCursor::PreviousCharacter);
    }
    auto link = linkifiedAnchorAt(cursor);
    if (link.isEmpty()) {
        link = unparsedAnchorAt(cursor);
    }
    if (link.isEmpty()) {
        // hlp = {};
    }
    return link;
}

QString NoteEdit::linkifiedAnchorAt(const QTextCursor &cursor)
{
    QTextBlock    block = cursor.block();
    QTextFragment startFrag;
    QUrl          url;

    QTextBlock::iterator centralIt = block.begin();
    for (centralIt = block.begin(); !(centralIt.atEnd()); ++centralIt) {
        startFrag = centralIt.fragment();
        if (startFrag.contains(cursor.position())) {
            // qDebug() << "found " << cursor.position() << " in fragment at " << startFrag.position() << " of length "
            //          << startFrag.length() << " text: " << startFrag.text();
            url = startFrag.charFormat().anchorHref();
            break;
        }
    }

    if (!url.isValid()) {
        return {};
    }

    int start = startFrag.position();
    int end   = start + startFrag.length();

    // look for the beginning
    auto beginIt = centralIt;
    while (beginIt != block.begin()) {
        --beginIt;
        QTextFragment frag = beginIt.fragment();
        if (frag.charFormat().anchorHref() == url) {
            start = frag.position();
        } else {
            break;
        }
    }

    auto endIt = centralIt;
    for (++endIt; !(endIt.atEnd()); ++endIt) {
        QTextFragment frag = endIt.fragment();
        if (frag.charFormat().anchorHref() == url) {
            end = frag.position() + frag.length();
        } else {
            break;
        }
    }

    hlp.block  = block;
    hlp.pos    = start - block.position();
    hlp.length = end - start;

    return block.text().sliced(hlp.pos, hlp.length);
}

QString NoteEdit::unparsedAnchorAt(const QTextCursor &cursor)
{
    bool startFound = false;
    bool endFound   = false;
    auto blockText  = cursor.block().text();
    int  startPos, endPos;
    startPos = endPos = cursor.positionInBlock();
    if (!blockText.length() || startPos >= blockText.size() || blockText[startPos].isSpace()) {
        return {};
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
                endFound = true;
            }
        }
    }
    hlp.block           = cursor.block();
    hlp.pos             = startPos;
    hlp.length          = endPos - startPos;
    QStringView matched = QStringView(blockText).mid(startPos, hlp.length);
    auto        indx    = matched.indexOf(QLatin1String("://"));
    if (indx == -1 && !matched.startsWith(QLatin1StringView("www."))) {
        return {};
    }
    int hostShift = 0;
    if (indx != -1) {
        static QRegularExpression schemeRE { ".*([a-zA-Z][a-zA-Z0-9+.-]*)$",
                                             QRegularExpression::InvertedGreedinessOption
                                                 | QRegularExpression::CaseInsensitiveOption };
        auto                      match = schemeRE.matchView(matched.sliced(0, indx));
        if (match.hasMatch()) {
            auto sz = match.capturedLength(1);
            // qDebug() << matched << matched.size() << match.capturedStart(1);
            hlp.length = matched.size() - match.capturedStart(1);
            hlp.pos    = hlp.pos + indx - sz;
            matched    = QStringView(blockText).mid(hlp.pos, hlp.length);
            hostShift  = match.capturedLength(1) + 3; // + ://
        }
    }
    static QRegularExpression re(R"(^((?:[a-zA-Z0-9-]+\.)+[a-zA-Z]{2,}(?:/[^\s?#]*(?:\?[^\s#]*)?(?:\#[^\s]*)?)?).*)",
                                 QRegularExpression::CaseInsensitiveOption);

    auto match = re.matchView(matched.sliced(hostShift));
    if (!match.hasMatch()) {
        return {};
    }
    hlp.length = match.capturedLength(1) + hostShift;
    return blockText.mid(hlp.pos, hlp.length);
}

void NoteEdit::mousePressEvent(QMouseEvent *e)
{
    QString url;
    if ((unconditionalLinks || e->modifiers() & Qt::ControlModifier) && e->button() == Qt::LeftButton
        && !(url = locateLinkAt(e->pos())).isEmpty()) {
        e->accept();
        QDesktopServices::openUrl(QUrl::fromUserInput(url));
        return;
    }
    QTextEdit::mousePressEvent(e);
}

void NoteEdit::mouseMoveEvent(QMouseEvent *e)
{
    QString url;
    if (!(url = locateLinkAt(e->pos())).isEmpty()) {
        viewport()->setCursor(Qt::PointingHandCursor);
        emit linkHovered();
    } else {
        if (viewport()->cursor().shape() == Qt::PointingHandCursor) {
            viewport()->setCursor(Qt::IBeamCursor);
            emit linkUnhovered();
        }
    }
    QTextEdit::mouseMoveEvent(e);
}

void NoteEdit::keyPressEvent(QKeyEvent *e)
{
    if (unconditionalLinks || e->key() == Qt::Key_Control) {
        setLinkHighlightEnabled(true); // enable mouse tracking
    }
    QTextEdit::keyPressEvent(e);
}

void NoteEdit::keyReleaseEvent(QKeyEvent *e)
{
    if (!unconditionalLinks && e->key() == Qt::Key_Control) {
        setLinkHighlightEnabled(false);
    }
    QTextEdit::keyPressEvent(e);
}

void QtNote::NoteEdit::setUnconditionalLinks(bool enabled)
{
    unconditionalLinks = enabled;
    setLinkHighlightEnabled(enabled); // we should rather
}

} // namespace QtNote
