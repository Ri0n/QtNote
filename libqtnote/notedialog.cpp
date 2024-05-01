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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QDesktopWidget>
#endif
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QScreen>
#include <QSettings>

#include "notedialog.h"
#include "notewidget.h"
#include "typeaheadfind.h"
#include "ui_notedialog.h"
#include "utils.h"

namespace QtNote {

NoteDialog::NoteDialog(NoteWidget *noteWidget) : QDialog(0), m_ui(new Ui::NoteDialog), noteWidget(noteWidget)
{
    setWindowFlags((windowFlags() ^ (Qt::Dialog | Qt::WindowContextHelpButtonHint)) | Qt::WindowSystemMenuHint
                   | Qt::CustomizeWindowHint | Qt::Window | Qt::WindowMinMaxButtonsHint);
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName("noteDlg");

    QHBoxLayout *l = new QHBoxLayout;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    l->setMargin(2);
#else
    l->setContentsMargins(2, 2, 2, 2);
#endif
    l->addWidget(noteWidget);
    setLayout(l);

    QRect rect;
    if (!noteWidget->noteId().isEmpty()) {
        rect = QSettings().value(QString("geometry.%1.%2").arg(noteWidget->storageId(), noteWidget->noteId())).toRect();

        Q_ASSERT(!NoteDialog::findDialog(noteWidget->storageId(), noteWidget->noteId()));
        NoteDialog::dialogs.insert(QPair<QString, QString>(noteWidget->storageId(), noteWidget->noteId()), this);
    }
    if (rect.isEmpty()) {
        QSize avail = screen()->availableSize() - sizeHint(); //   QApplication::desktop()->size()
        int   x = avail.width() / 4 + (QRandomGenerator::global()->generate() / (float)RAND_MAX) * avail.width() / 2;
        int   y = avail.height() / 4 + (QRandomGenerator::global()->generate() / (float)RAND_MAX) * avail.height() / 2;
        move(x, y);
    } else {
        setGeometry(rect);
    }

    noteWidget->setFocus(Qt::OtherFocusReason);

    connect(noteWidget, &NoteWidget::trashRequested, this, &NoteDialog::trashRequested);
    connect(noteWidget, &NoteWidget::noteIdChanged, this, [this](const QString &oldId, const QString &newId) {
        if (oldId.isEmpty() && !newId.isEmpty()) {
            NoteDialog::dialogs.insert(QPair<QString, QString>(this->noteWidget->storageId(), newId), this);
        }
    });
    connect(noteWidget, &NoteWidget::firstLineChanged, this, &NoteDialog::firstLineChanged);

    firstLineChanged();
}

NoteDialog::~NoteDialog() { delete m_ui; }

NoteDialog *NoteDialog::findDialog(const QString &storageId, const QString &noteId)
{
    return NoteDialog::dialogs.value(QPair<QString, QString>(storageId, noteId));
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
    QDialog::changeEvent(e);
}

void NoteDialog::trashRequested()
{
    noteWidget->setTrashRequested(true); // in case when called outside
    close();
}

void NoteDialog::done(int r)
{
    noteWidget->disconnect(this);
    if (!noteWidget->isTrashRequested()) { // do it first to update noteWidget::noteId
        noteWidget->save();
    }
    if (!noteWidget->noteId().isEmpty()) {
        QSettings s;
        QString   key = QString("geometry.%1.%2").arg(noteWidget->storageId(), noteWidget->noteId());
        if (noteWidget->isTrashRequested()) {
            s.remove(key);
        } else {
            s.setValue(key, geometry());
        }
        NoteDialog::dialogs.remove(QPair<QString, QString>(noteWidget->storageId(), noteWidget->noteId()));
    }
    QDialog::done(r);
}

void NoteDialog::firstLineChanged()
{
    QString l = noteWidget->firstLine().trimmed();
    setWindowTitle(Utils::cuttedDots(l.isEmpty() ? tr("[No Title]") : l, 256));
}

QHash<QPair<QString, QString>, NoteDialog *> NoteDialog::dialogs;

} // namespace QtNote
