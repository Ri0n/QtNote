#include "notedialog.h"
#include "ui_notedialog.h"
#include "note.h"

NoteDialog::NoteDialog(QWidget *parent) :
	QDialog(parent),
    m_ui(new Ui::NoteDialog)
{
    m_ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	m_ui->trashBtn->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
	m_ui->noteEdit->setFocus(Qt::OtherFocusReason);

	connect(m_ui->trashBtn, SIGNAL(clicked()), SLOT(trashClicked()));
}

NoteDialog::~NoteDialog()
{
    delete m_ui;
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

void NoteDialog::trashClicked()
{
	((Note *)parent())->toTrash();
	reject();
}

void NoteDialog::setText(QString text)
{
	m_ui->noteEdit->setPlainText(text);
}

QString NoteDialog::text()
{
	return m_ui->noteEdit->toPlainText();
}
