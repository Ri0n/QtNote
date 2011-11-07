#include "notemanagerdlg.h"
#include "ui_notemanagerdlg.h"
#include "notemanagermodel.h"
#include "mainwidget.h"

NoteManagerDlg::NoteManagerDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NoteManagerDlg)
{
    ui->setupUi(this);
	model = new NoteManagerModel(this);
	ui->notesTree->setModel(model);
	int sumCount = 0;
	for (int i = 0, cnt = model->rowCount(); i < cnt; i++) {
		sumCount += model->rowCount(model->index(i, 0));
	}
	setWindowTitle(tr("Note Manager (%1)").arg(tr("%n notes", 0, sumCount)));
	connect(ui->notesTree, SIGNAL(doubleClicked(QModelIndex)), SLOT(itemDoubleClicked(QModelIndex)));
}

NoteManagerDlg::~NoteManagerDlg()
{
    delete ui;
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
	QString noteId = model->data(index, NoteManagerModel::NoteId).toString();
	if (!noteId.isEmpty()) {
		((Widget*)parent())->showNoteDialog(model->data(index, NoteManagerModel::StorageId).toString(), noteId);
	}
}
