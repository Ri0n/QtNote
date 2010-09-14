#include "notemanagerdlg.h"
#include "ui_notemanagerdlg.h"
#include "notemanagermodel.h"

NoteManagerDlg::NoteManagerDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NoteManagerDlg)
{
    ui->setupUi(this);
	model = new NoteManagerModel(this);
	ui->notesTree->setModel(model);
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
