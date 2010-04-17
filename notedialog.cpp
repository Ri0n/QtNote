#include "notedialog.h"
#include "ui_notedialog.h"
#include "note.h"

NoteDialog::NoteDialog(QWidget *parent, const QString &storageId, const QString &noteId) :
	QDialog(parent),
	m_ui(new Ui::NoteDialog),
	storageId_(storageId),
	noteId_(noteId),
	trashRequested_(false)
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

void NoteDialog::done(int r)
{
	qDebug("saving: %d", r);
	if (trashRequested_) {
		emit trashRequested(storageId_, noteId_);
	} else {
		emit saveRequested(storageId_, noteId_, text());
	}
	QDialog::done(r);
}

void NoteDialog::trashClicked()
{
	trashRequested_ = true;
	close();
}

void NoteDialog::setText(QString text)
{
	m_ui->noteEdit->setPlainText(text);
}

QString NoteDialog::text()
{
	return m_ui->noteEdit->toPlainText();
}

bool NoteDialog::checkOwnership(const QString &storageId,
								const QString &noteId) const
{
	return storageId == storageId_ && noteId == noteId_;
}
