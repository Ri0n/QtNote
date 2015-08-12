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

#include <QItemSelection>
#include <QPropertyAnimation>
#include <QSettings>

#include "notemanagerdlg.h"
#include "ui_notemanagerdlg.h"
#include "notesmodel.h"
#include "notewidget.h"
#include "qtnote.h"
#include "notessearchmodel.h"

namespace QtNote {


class SearchOptsAnim : public QObject {
    Q_OBJECT

    QWidget *focusWidget;
    QWidget *animWidget;
    QPropertyAnimation *animation;
public:
    SearchOptsAnim(QWidget *focusWidget, QWidget *animWidget, QObject *parent) :
        QObject(parent),
        focusWidget(focusWidget),
        animWidget(animWidget)
    {
        focusWidget->installEventFilter(this);
        animWidget->installEventFilter(this);
        animation = new QPropertyAnimation(animWidget, "maximumHeight", this);
        animation->setDuration(200);
    }

    bool eventFilter(QObject *obj, QEvent *event)
    {
        if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) {
            QWidget *w = QApplication::focusWidget();
            if (focusWidget == w || animWidget == w) {
                if (!animWidget->maximumHeight()) {
                    animation->setStartValue(0);
                    animation->setEndValue(animWidget->sizeHint().height());
                    animation->start();
                }
            } else {
                if (animWidget->maximumHeight()) {
                    animation->setStartValue(animWidget->sizeHint().height());
                    animation->setEndValue(0);
                    animation->start();
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }
};


NoteManagerDlg::NoteManagerDlg(Main *qtnote) :
	QDialog(0),
	ui(new Ui::NoteManagerDlg),
	qtnote(qtnote)
{
    ui->setupUi(this);
	setWindowIcon(QIcon(":/icons/manager"));
	setAttribute(Qt::WA_DeleteOnClose);
    ui->ckSearchInText->setMaximumHeight(0);
    new SearchOptsAnim(ui->leFilter, ui->ckSearchInText, this);

	model = new NotesModel(this);
    searchModel = new NotesSearchModel(this);
    searchModel->setSourceModel(model);
    searchModel->setDynamicSortFilter(true);
    searchModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->notesTree->setModel(searchModel);

    QSettings s;
    bool expandAll = false;
    QStringList expanded;
    if (!s.contains("nm-expanded")) {
        expandAll = true;
    } else {
        expanded = QSettings().value("nm-expanded").toStringList();
    }
	for (int i = 0, cnt = model->rowCount(); i < cnt; i++) {
        auto index = model->index(i, 0);
        if (expandAll || expanded.contains(model->data(index, NotesModel::StorageIdRole).toString())) {
            ui->notesTree->setExpanded(searchModel->mapFromSource(index), true);
        }
	}

    updateStats();
	connect(ui->notesTree, SIGNAL(doubleClicked(QModelIndex)), SLOT(itemDoubleClicked(QModelIndex)));
    connect(ui->notesTree->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), SLOT(currentRowChanged(QModelIndex,QModelIndex)));
    connect(ui->leFilter, SIGNAL(textChanged(QString)), searchModel, SLOT(setSearchText(QString)));
    connect(ui->ckSearchInText, SIGNAL(clicked(bool)), searchModel, SLOT(setSearchInBody(bool)));
    connect(model, SIGNAL(statsChanged()), SLOT(updateStats()));
}

NoteManagerDlg::~NoteManagerDlg()
{
    QSettings s;
    QStringList expanded;
    for (int i = 0, cnt = model->rowCount(); i < cnt; i++) {
        auto index = model->index(i, 0);
        if (ui->notesTree->isExpanded(searchModel->mapFromSource(index))) {
            expanded << model->data(index, NotesModel::StorageIdRole).toString();
        }
    }
    s.setValue("nm-expanded", expanded);
    delete ui;
}

void NoteManagerDlg::updateStats()
{
    int sumCount = 0;
    for (int i = 0, cnt = model->rowCount(); i < cnt; i++) {
        sumCount += model->rowCount(model->index(i, 0));
    }
    setWindowTitle(tr("Note Manager (%1)").arg(tr("%n notes", 0, sumCount)));
}

void NoteManagerDlg::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void NoteManagerDlg::itemDoubleClicked(const QModelIndex &index)
{
    auto srcIndex = searchModel->mapToSource(index);
    QString noteId = model->data(srcIndex, NotesModel::NoteIdRole).toString();
	if (!noteId.isEmpty()) {
        emit showNoteRequested(model->data(srcIndex, NotesModel::StorageIdRole).toString(), noteId);
	}
}

void NoteManagerDlg::currentRowChanged(const QModelIndex &current, const QModelIndex &previous)
{
	// TODO save previous?
	Q_UNUSED(previous)
    auto srcCurrent = searchModel->mapToSource(current);
    if (srcCurrent.data(NotesModel::ItemTypeRole).toInt() != NotesModel::ItemNote) {
		return;
	}
    NoteWidget *nw = qtnote->noteWidget(srcCurrent.data(NotesModel::StorageIdRole).toString(),
                                        srcCurrent.data(NotesModel::NoteIdRole).toString());

    /* probably it's ok to just return if nw=0. stirage will be anyway invalidated on errors */
    if (!nw) {
        return;
    }

	if (ui->splitter->count() > 1) {
		delete ui->splitter->widget(ui->splitter->count() - 1);
	} else {
		resize(700, 400);
	}
	ui->splitter->addWidget(nw);
	ui->splitter->setStretchFactor(0, 0);
	ui->splitter->setStretchFactor(1, 1);
}

} // namespace QtNote

#include "notemanagerdlg.moc"
