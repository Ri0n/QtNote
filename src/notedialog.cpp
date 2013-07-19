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
#include <QPrinter>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>

#include "utils.h"

NoteDialog::NoteDialog(const QString &storageId, const QString &noteId) :
	QDialog(0),
	m_ui(new Ui::NoteDialog),
	storageId_(storageId),
	noteId_(noteId),
	trashRequested_(false)
{
	Q_ASSERT(!NoteDialog::findDialog(storageId, noteId));
	NoteDialog::dialogs.insert(QPair<QString,QString>(storageId, noteId), this);

	setWindowFlags((windowFlags() ^ (Qt::Dialog | Qt::WindowContextHelpButtonHint)) |
				   Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Window | Qt::WindowMinMaxButtonsHint);
	m_ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	m_ui->noteEdit->setFocus(Qt::OtherFocusReason);
	m_ui->saveBtn->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

	QSize avail = QApplication::desktop()->size() - sizeHint();
	int x = avail.width() / 4 + (qrand()/(float)RAND_MAX)*avail.width() / 2;
	int y = avail.height() / 4 + (qrand()/(float)RAND_MAX)*avail.height() / 2;
	move(x, y);

	titleCharFormat_.setFontPointSize(m_ui->noteEdit->font().pointSize() * 1.5);
	titleCharFormat_.setForeground(QBrush(QColor(255,0,0)));
	secondLineCharFormat_ = m_ui->noteEdit->currentCharFormat();

	autosaveTimer_.setInterval(10000);
	connect(&autosaveTimer_, SIGNAL(timeout()), SLOT(autosave()));
	//connect(parent, SIGNAL(destroyed()), SLOT(close()));
	connect(m_ui->noteEdit, SIGNAL(textChanged()), SLOT(textChanged()));
	connect(m_ui->trashBtn, SIGNAL(clicked()), SLOT(trashClicked()));
	connect(m_ui->copyBtn, SIGNAL(clicked()), SLOT(copyClicked()));

	m_ui->noteEdit->setText(""); // to force update event
	if (!noteId.isEmpty()) {
		QRect rect = QSettings().value(QString("geometry.%1.%2")
				  .arg(storageId_, noteId_)).toRect();
		if (!rect.isEmpty()) {
			setGeometry(rect);
		}
	}
}

NoteDialog::~NoteDialog()
{
	autosaveTimer_.stop();
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
		emit trash();
	} else {
		changed_ = false;
		emit save();
		if (!noteId_.isEmpty()) {
			QSettings s;
			s.setValue(QString("geometry.%1.%2")
				   .arg(storageId_, noteId_), geometry());
		}
	}
	NoteDialog::dialogs.remove(QPair<QString,QString>(storageId_, noteId_));
	QDialog::done(r);
}

void NoteDialog::autosave()
{
	if (!text().isEmpty() && changed_) {
		changed_ = false;
		emit save();
	} else {
		autosaveTimer_.stop(); // stop until next text change
	}
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

void NoteDialog::textChanged()
{
	changed_ = true;
	if (!autosaveTimer_.isActive()) {
		autosaveTimer_.start();
	}
	QTextDocument *doc = m_ui->noteEdit->document();
	QTextBlock firstBlock = doc->begin();

	QString firstLine = firstBlock.text();
	if (firstLine != firstLine_ || firstLine.isEmpty()) {
		firstLine_ = firstLine;
		setWindowTitle(
			Utils::cuttedDots(firstLine.isEmpty() ?
							  tr("[No Title]") :
							  text().section('\n', 0, 0).trimmed(), 256)
		);
	}
}

void NoteDialog::setText(QString text)
{
	m_ui->noteEdit->setPlainText(text);
	changed_ = false; // mark as unchanged since its not user input.
	autosaveTimer_.stop(); // timer not required atm
}

QString NoteDialog::text()
{
	return m_ui->noteEdit->toPlainText().trimmed();
}

void NoteDialog::setAcceptRichText(bool state)
{
	m_ui->noteEdit->setAcceptRichText(state);
}

void NoteDialog::setNoteId(const QString &noteId)
{
	NoteDialog::dialogs.remove(NoteDialog::dialogs.key(this));
	noteId_ = noteId;
	NoteDialog::dialogs.insert(QPair<QString,QString>(storageId_, noteId_), this);
}


void NoteDialog::on_printBtn_clicked()
{
	QPrinter printer;
	if (!text().isEmpty()) {
		m_ui->noteEdit->print(&printer);
	}
}

void NoteDialog::on_saveBtn_clicked()
{
	if (extFileName_.isEmpty() || !QFile::exists(extFileName_)) {
		extFileName_ = QFileDialog::getSaveFileName(this, tr("Save Note As"),
			QDesktopServices::storageLocation(QDesktopServices::DesktopLocation));
	}
	if (!extFileName_.isEmpty()) {
		QFile f(extFileName_);
		if (f.open(QIODevice::WriteOnly)) {
			QFileInfo fi(extFileName_);
			if ((QStringList() << "html" << "htm" << "xhtml" << "xml")
					.contains(fi.suffix().toLower())) {
				f.write(m_ui->noteEdit->toHtml().toLocal8Bit());
			} else {
				f.write(m_ui->noteEdit->toPlainText().toLocal8Bit());
			}
			f.close();
		}
	}
}

QHash< QPair<QString,QString>, NoteDialog* > NoteDialog::dialogs;
