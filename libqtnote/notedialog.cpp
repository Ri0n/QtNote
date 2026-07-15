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

#include "deintegrationinterface.h"
#include "notedialog.h"
#include "notestorage.h"
#include "notewidget.h"
#include "qtnote.h"
#include "typeaheadfind.h"
#include "ui_notedialog.h"
#include "utils.h"

namespace QtNote {

NoteDialog::NoteDialog(NoteWidget *noteWidget, Main *main) :
    QDialog(0), m_ui(new Ui::NoteDialog), noteWidget(noteWidget), main(main)
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
    bool  geometryHandled = false;
    if (!noteWidget->note().id().isEmpty()) {
        auto          note          = noteWidget->note();
        const QString geometryKey   = QString("geometry.%1.%2").arg(note.storageId(), note.id());
        const auto    restoreResult = main->restoreWindowGeometry(this, geometryKey);
        geometryHandled             = restoreResult != WindowGeometryRestoreResult::Unsupported;
        rect                        = QSettings().value(geometryKey).toRect();
        if (restoreResult == WindowGeometryRestoreResult::Pending) {
            // Wayland applications cannot choose their global position, but they
            // still own their initial size while KWin restores frameGeometry.
            if (rect.isValid())
                resize(rect.size());
            else
                resize(sizeHint());
            rect = {};
        } else if (!geometryHandled) {
            if (!rect.isValid() || !screen()->geometry().contains(rect)) {
                rect = {};
            }
        }

        Q_ASSERT(!NoteDialog::findDialog(noteWidget->note().storageId(), noteWidget->note().id()));
        NoteDialog::dialogs.insert(QPair<QString, QString>(noteWidget->note().storageId(), noteWidget->note().id()),
                                   this);
    }
    if (rect.isEmpty() && !geometryHandled) {
        QSize avail = screen()->availableSize() - sizeHint(); //   QApplication::desktop()->size()
        int   x     = QRandomGenerator::global()->bounded(avail.width() / 4, avail.width() / 2);
        int   y     = QRandomGenerator::global()->bounded(avail.height() / 4, avail.height() / 2);
        move(x, y);
    } else {
        setGeometry(rect);
    }

    noteWidget->setFocus(Qt::OtherFocusReason);

    connect(noteWidget, &NoteWidget::trashRequested, this, &NoteDialog::trashRequested);
    connect(noteWidget, &NoteWidget::noteIdChanged, this, [this](const QString &oldId, const QString &newId) {
        if (oldId.isEmpty() && !newId.isEmpty()) {
            NoteDialog::dialogs.insert(QPair<QString, QString>(this->noteWidget->note().storageId(), newId), this);
            const QString key = QString("geometry.%1.%2").arg(this->noteWidget->note().storageId(), newId);
            this->main->restoreWindowGeometry(this, key);
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

QList<NoteDialog *> NoteDialog::openDialogs() { return NoteDialog::dialogs.values(); }

void NoteDialog::registerWindowGeometry()
{
    const auto note = noteWidget->note();
    if (note.id().isEmpty())
        return;
    main->restoreWindowGeometry(this, QString("geometry.%1.%2").arg(note.storageId(), note.id()));
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
    noteWidget->discardDraft();
    close();
}

void NoteDialog::done(int r)
{
    noteWidget->disconnect(this);
    if (!noteWidget->isTrashRequested() && !noteWidget->prepareToClose()) {
        return;
    }
    if (!noteWidget->note().id().isEmpty()) {
        QSettings s;
        QString   key
            = QString("geometry.%1.%2").arg(noteWidget->note().storage()->systemName(), noteWidget->note().id());
        if (noteWidget->isTrashRequested()) {
            main->removeWindowGeometry(key);
            s.remove(key);
        } else {
            if (!main->saveWindowGeometry(this, key))
                s.setValue(key, geometry());
        }
        NoteDialog::dialogs.remove(
            QPair<QString, QString>(noteWidget->note().storage()->systemName(), noteWidget->note().id()));
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
