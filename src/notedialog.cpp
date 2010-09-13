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

#include "notedialog.h"
#include "ui_notedialog.h"
#include "note.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QClipboard>
#include <QTimer>
#include <QTextFragment>
#include "utils.h"

NoteDialog::NoteDialog(QWidget *parent, const QString &storageId, const QString &noteId) :
	QDialog(0),
	m_ui(new Ui::NoteDialog),
	storageId_(storageId),
	noteId_(noteId),
	trashRequested_(false)
{
	Q_ASSERT(!NoteDialog::findDialog(storageId, noteId));
	NoteDialog::dialogs.insert(QPair<QString,QString>(storageId, noteId), this);

	setWindowFlags((windowFlags() ^ (Qt::Dialog | Qt::WindowContextHelpButtonHint)) |
				   Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Window);
	m_ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	m_ui->noteEdit->setFocus(Qt::OtherFocusReason);

	QSize avail = QApplication::desktop()->size() - sizeHint();
	int x = avail.width() / 4 + (qrand()/(float)RAND_MAX)*avail.width() / 2;
	int y = avail.height() / 4 + (qrand()/(float)RAND_MAX)*avail.height() / 2;
	move(x, y);

	titleCharFormat_.setFontPointSize(m_ui->noteEdit->font().pointSize() * 1.5);
	titleCharFormat_.setForeground(QBrush(QColor(255,0,0)));
	secondLineCharFormat_ = m_ui->noteEdit->currentCharFormat();

	connect(parent, SIGNAL(destroyed()), SLOT(close()));
	connect(m_ui->noteEdit->document(), SIGNAL(contentsChange(int,int,int)),
			SLOT(contentsChange(int,int,int)));
	connect(m_ui->trashBtn, SIGNAL(clicked()), SLOT(trashClicked()));
	connect(m_ui->copyBtn, SIGNAL(clicked()), SLOT(copyClicked()));

	m_ui->noteEdit->setText(""); // to force update event
}

NoteDialog::~NoteDialog()
{
    delete m_ui;
}

NoteDialog* NoteDialog::findDialog(const QString &storageId, const QString &noteId)
{
	return NoteDialog::dialogs.value(QPair<QString,QString>(storageId, noteId));
}

void NoteDialog::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void NoteDialog::done(int r)
{
	//qDebug("saving: %d", r);
	if (trashRequested_) {
		emit trashRequested(storageId_, noteId_);
	} else {
		emit saveRequested(storageId_, noteId_, text().trimmed());
	}
	NoteDialog::dialogs.remove(QPair<QString,QString>(storageId_, noteId_));
	QDialog::done(r);
}

void NoteDialog::trashClicked()
{
	trashRequested_ = true;
	close();
}

void NoteDialog::copyClicked()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(text());
}

void NoteDialog::contentsChange(int position, int charsRemoved, int charsAdded)
{
	Q_UNUSED(charsRemoved)
	// first lets try remove space at the begining of line
	// do this immediately to hide visual effects
	QTextDocument *doc = m_ui->noteEdit->document();
	QTextCursor curBak = m_ui->noteEdit->textCursor();
	QTextCursor cur(doc);

	cur.setPosition(0);
	// half of workaround for bug http://bugreports.qt.nokia.com/browse/QTBUG-3495
	if (doc->characterAt(position + charsAdded) == '\0') {
		charsAdded--;
	}

	int n = 0;
	while (n < doc->characterCount() - 1 && doc->characterAt(n).isSpace()) {
		n++;
	}
	if (n) {
		//qDebug("removing empty symbols %d from %d", n, doc->characterCount());
		curBak.setPosition(0);
		cur.setPosition(n, QTextCursor::KeepAnchor);
		cur.deleteChar();
		if (n > position) { // something was removed from start of text
			if (n > position + charsAdded) {
				charsAdded = 0;
			} else {
				charsAdded -= (n - position);
			}
			position = 0;
		} else {
			position -= n;
		}
	}

	QTextBlock firstBlock = doc->begin();
	cur.setPosition(firstBlock.length() - 1, QTextCursor::KeepAnchor);
	cur.setCharFormat(titleCharFormat_);
	if (charsAdded > 0 && firstBlock.contains(position)) {
		cur.setPosition(position);
		cur.setPosition(position + charsAdded, QTextCursor::KeepAnchor);
		QString addText = cur.selectedText();
		//int pos = addText.indexOf(QChar::Separator_Paragraph); // doesn't work for unknown reason
		int pos = 0;
		for (int l=addText.size();
			pos<l && addText[pos].category() != QChar::Separator_Paragraph;
			pos++);
		if (pos < addText.size()) {
			QTextBlock b = firstBlock.next();
			if (b.isValid()) {
				int firstIndex = firstBlock.begin().fragment().charFormatIndex();
				int currentIndex = firstIndex;
				int lastPos = b.position();
				// computing length of text after first line with the same format as title format
				while (b.isValid() && currentIndex == firstIndex) {
					QTextBlock::iterator it;
					for (it = b.begin(); !(it.atEnd()); ++it) {
						QTextFragment fr = it.fragment();
						if (fr.isValid()) {
							currentIndex = fr.charFormatIndex();
							if (currentIndex == firstIndex) {
								lastPos = fr.position() + fr.length();
							} else {
								break;
							}
						}
					}
					b = b.next();
				}
				// setting format to second line format or some default
				cur.setPosition(position + pos);
				cur.setPosition(lastPos, QTextCursor::KeepAnchor);
				cur.setCharFormat(secondLineCharFormat_);
				curBak.setCharFormat(secondLineCharFormat_);
			}
		}
	}
	QString firstLine = firstBlock.text();
	if (firstLine != firstLine_ || firstLine.isEmpty()) {
		firstLine_ = firstLine;
		setWindowTitle(
			Utils::cuttedDots(firstLine.isEmpty() ?
							  tr("[Empty Note]") :
							  text().section('\n', 0, 0).trimmed(), 256)
		);
	}
	m_ui->noteEdit->setTextCursor(curBak);
}

void NoteDialog::setText(QString text)
{
	m_ui->noteEdit->setPlainText(text);
}

QString NoteDialog::text()
{
	return m_ui->noteEdit->toPlainText();
}

void NoteDialog::setAcceptRichText(bool state)
{
	m_ui->noteEdit->setAcceptRichText(state);
}

QHash< QPair<QString,QString>, NoteDialog* > NoteDialog::dialogs;
