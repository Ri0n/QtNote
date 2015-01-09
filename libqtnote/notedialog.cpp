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

#include <QApplication>
#include <QDesktopWidget>
#include <QSettings>
#include <QHBoxLayout>


#include "utils.h"
#include "typeaheadfind.h"
#include "notedialog.h"
#include "ui_notedialog.h"
#include "note.h"
#include "notewidget.h"
#include "utils.h"

namespace QtNote {

NoteDialog::NoteDialog(NoteWidget *noteWidget) :
	QDialog(0),
	_trashRequested(false),
	m_ui(new Ui::NoteDialog),
	noteWidget(noteWidget)
{
	setWindowFlags((windowFlags() ^ (Qt::Dialog | Qt::WindowContextHelpButtonHint)) |
				   Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Window | Qt::WindowMinMaxButtonsHint);
	m_ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	setObjectName("noteDlg");
	setWindowIcon(QIcon(":/icons/trayicon"));

	QHBoxLayout *l = new QHBoxLayout;
	l->setMargin(2);
	l->addWidget(noteWidget);
	setLayout(l);

	QRect rect;
	if (!noteWidget->noteId().isEmpty()) {
		rect = QSettings().value(QString("geometry.%1.%2")
								 .arg(noteWidget->storageId(), noteWidget->noteId())).toRect();

		Q_ASSERT(!NoteDialog::findDialog(noteWidget->storageId(), noteWidget->noteId()));
		NoteDialog::dialogs.insert(QPair<QString,QString>(noteWidget->storageId(), noteWidget->noteId()), this);
	}
	if (rect.isEmpty()) {
		QSize avail = QApplication::desktop()->size() - sizeHint();
		int x = avail.width() / 4 + (qrand()/(float)RAND_MAX)*avail.width() / 2;
		int y = avail.height() / 4 + (qrand()/(float)RAND_MAX)*avail.height() / 2;
		move(x, y);
	} else {
		setGeometry(rect);
	}

	noteWidget->setFocus(Qt::OtherFocusReason);

	connect(noteWidget, SIGNAL(trashRequested()), SLOT(trashRequested()));
	connect(noteWidget, SIGNAL(noteIdChanged(QString,QString)), SLOT(noteIdChanged(QString,QString)));
	connect(noteWidget, SIGNAL(firstLineChanged()), SLOT(firstLineChanged()));

	firstLineChanged();
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

void NoteDialog::trashRequested()
{
	_trashRequested  = true;
	close();
}

void NoteDialog::done(int r)
{
	noteWidget->disconnect(this);
	if (!_trashRequested) { // do it first to update noteWidget::noteId
		emit noteWidget->save();
	}
	if (!noteWidget->noteId().isEmpty()) {
		QSettings s;
		QString key = QString("geometry.%1.%2")
				.arg(noteWidget->storageId(), noteWidget->noteId());
		if (_trashRequested) {
			s.remove(key);
		} else {
			s.setValue(key, geometry());
		}
		NoteDialog::dialogs.remove(QPair<QString,QString>(noteWidget->storageId(), noteWidget->noteId()));
	}
	QDialog::done(r);
}

void NoteDialog::firstLineChanged()
{
	QString l = noteWidget->firstLine().trimmed();
	setWindowTitle(Utils::cuttedDots(l.isEmpty() ? tr("[No Title]") : l, 256));
}

void NoteDialog::noteIdChanged(const QString &oldId, const QString &newId)
{
	if (oldId.isEmpty() && !newId.isEmpty()) {
		NoteDialog::dialogs.insert(QPair<QString,QString>(noteWidget->storageId(), newId), this);
	}
}


QHash< QPair<QString,QString>, NoteDialog* > NoteDialog::dialogs;

} // namespace QtNote
