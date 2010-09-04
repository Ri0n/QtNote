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

	connect(parent, SIGNAL(destroyed()), SLOT(close()));
	connect(m_ui->trashBtn, SIGNAL(clicked()), SLOT(trashClicked()));
	connect(m_ui->copyBtn, SIGNAL(clicked()), SLOT(copyClicked()));
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

void NoteDialog::setText(QString text)
{
	m_ui->noteEdit->setPlainText(text);
}

QString NoteDialog::text()
{
	return m_ui->noteEdit->toPlainText();
}

QHash< QPair<QString,QString>, NoteDialog* > NoteDialog::dialogs;
