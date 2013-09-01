#include <QClipboard>
#include <QTimer>
#include <QTextFragment>
#include <QPrinter>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QMessageBox>

#include "notewidget.h"
#include "ui_notewidget.h"
#include "typeaheadfind.h"

NoteWidget::NoteWidget(const QString &storageId, const QString &noteId) :
	ui(new Ui::NoteWidget),
	storageId_(storageId),
	noteId_(noteId),
	trashRequested_(false)
{
	ui->setupUi(this);

	setFocusProxy(ui->noteEdit);
	ui->saveBtn->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

	QHBoxLayout *hb3a = new QHBoxLayout();
	findBar = new TypeAheadFindBar(ui->noteEdit, QString::null, this);
	hb3a->addWidget(findBar);
	ui->noteLayout->addLayout(hb3a);

	autosaveTimer_.setInterval(10000);
	connect(&autosaveTimer_, SIGNAL(timeout()), SLOT(autosave()));
	//connect(parent, SIGNAL(destroyed()), SLOT(close()));
	connect(ui->noteEdit, SIGNAL(textChanged()), SLOT(textChanged()));
	connect(ui->copyBtn, SIGNAL(clicked()), SLOT(copyClicked()));
	connect(ui->findBtn, SIGNAL(clicked()), findBar, SLOT(toggleVisibility()));

	ui->noteEdit->setText(""); // to force update event

}

NoteWidget::~NoteWidget()
{
	autosaveTimer_.stop();
	delete ui;
}

void NoteWidget::changeEvent(QEvent *e)
{
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}


void NoteWidget::keyPressEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Escape && findBar->isVisible()) {
		findBar->hide();
		return;
	}
	QWidget::keyPressEvent(event);
}

void NoteWidget::autosave()
{
	if (!text().isEmpty() && changed_) {
		changed_ = false;
		emit saveRequested();
	} else {
		autosaveTimer_.stop(); // stop until next text change
	}
}

void NoteWidget::copyClicked()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(text());
}

void NoteWidget::textChanged()
{
	changed_ = true;
	if (!autosaveTimer_.isActive()) {
		autosaveTimer_.start();
	}
	QTextDocument *doc = ui->noteEdit->document();
	QTextBlock firstBlock = doc->begin();

	QString firstLine = firstBlock.text();
	if (firstLine != firstLine_ || firstLine.isEmpty()) {
		firstLine_ = firstLine;
		emit firstLineChanged();
	}
}

void NoteWidget::setText(QString text)
{
	ui->noteEdit->setPlainText(text);
	changed_ = false; // mark as unchanged since its not user input.
	autosaveTimer_.stop(); // timer not required atm
}

QString NoteWidget::text()
{
	return ui->noteEdit->toPlainText().trimmed();
}

void NoteWidget::setAcceptRichText(bool state)
{
	ui->noteEdit->setAcceptRichText(state);
}

void NoteWidget::setNoteId(const QString &noteId)
{
	QString old = noteId_;
	noteId_ = noteId;
	emit noteIdChanged(old, noteId);
}

void NoteWidget::on_printBtn_clicked()
{
	QPrinter printer;
	if (!text().isEmpty()) {
		ui->noteEdit->print(&printer);
	}
}

void NoteWidget::on_saveBtn_clicked()
{
	if (extFileName_.isEmpty() || !QFile::exists(extFileName_)) {
		extFileName_ = QFileDialog::getSaveFileName(this, tr("Save Note As"),
#if QT_VERSION < 0x050000
			QDesktopServices::storageLocation(QDesktopServices::DesktopLocation)
#else
			QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
#endif
													);
	}
	if (!extFileName_.isEmpty()) {
		QFile f(extFileName_);
		if (f.open(QIODevice::WriteOnly)) {
			QFileInfo fi(extFileName_);
			QString text;
			if ((QStringList() << "html" << "htm" << "xhtml" << "xml")
					.contains(fi.suffix().toLower())) {
				text = ui->noteEdit->toHtml();
			} else {
				text = ui->noteEdit->toPlainText();
			}
			QByteArray data = text.toLocal8Bit();
#ifdef Q_OS_WIN
			data = data.replace("\n", 1, "\r\n", 2);
#endif
			f.write(data);
			f.close();
		}
	}
}

void NoteWidget::on_trashBtn_clicked()
{
	QSettings s;
	if (!text().isEmpty() && s.value("ui.ask-on-delete", true).toBool() &&
		QMessageBox::question(0, tr("Deletion confirmation"),
							  tr("Are you sure want to delete this note?"),
							  QMessageBox::Yes | QMessageBox::No) !=
		QMessageBox::Yes) {
		return;
	}
	emit trashRequested();
}
