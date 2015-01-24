#include <QClipboard>
#include <QTextFragment>
#include <QPrinter>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QMessageBox>
#include <QToolButton>

#include "notewidget.h"
#include "ui_notewidget.h"
#include "typeaheadfind.h"

namespace QtNote {

struct ActData {
	const char* icon;
	const char* text;
	const char* toolTip;
	const char* shortcut;
};

static struct {
	ActData save;
	ActData copy;
	ActData print;
	ActData find;
	ActData replace;
	ActData trash;
} staticActData = {
	.save   =  {"",              QT_TR_NOOP("Save"),   QT_TR_NOOP("Save note to file"),      "Ctrl+S"},
	.copy   =  {":/icons/copy",  QT_TR_NOOP("Copy"),   QT_TR_NOOP("Copy note to clipboard"), "Ctrl+Shift+C"},
	.print  =  {":/icons/print", QT_TR_NOOP("Print"),  QT_TR_NOOP("Print note"),             "Ctrl+P"},
	.find   =  {":/icons/find",  QT_TR_NOOP("Find"),   QT_TR_NOOP("Find text in note"),      "Ctrl+F"},
	.replace = {":/icons/replace-text",  QT_TR_NOOP("Replace"),QT_TR_NOOP("Replace text in note"),      "Ctrl+R"},
	.trash  =  {":/icons/trash", QT_TR_NOOP("Delete"), QT_TR_NOOP("Delete note"),            "Ctrl+D"}
};

NoteWidget::NoteWidget(const QString &storageId, const QString &noteId) :
	ui(new Ui::NoteWidget),
	_storageId(storageId),
	noteId_(noteId),
	_trashRequested(false)
{
	ui->setupUi(this);

	setFocusProxy(ui->noteEdit);
	//ui->saveBtn->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

	QHBoxLayout *hb3a = new QHBoxLayout();
	findBar = new TypeAheadFindBar(ui->noteEdit, QString::null, this);
	hb3a->addWidget(findBar);
	ui->noteLayout->addLayout(hb3a);

	_autosaveTimer.setInterval(10000);
	_lastChangeElapsed.start();
	connect(&_autosaveTimer, SIGNAL(timeout()), SLOT(autosave()));
	//connect(parent, SIGNAL(destroyed()), SLOT(close()));
	
	QToolBar *tbar = new QToolBar(this);
	ui->toolbarLayout->addWidget(tbar);
	
	QAction *act = initAction(staticActData.save);
	act->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
	tbar->addAction(act);
	connect(act, SIGNAL(triggered()), SLOT(onSaveClicked()));

	act = initAction(staticActData.copy);
	tbar->addAction(act);
	connect(act, SIGNAL(triggered()), SLOT(onCopyClicked()));

	act = initAction(staticActData.print);
	tbar->addAction(act);
	connect(act, SIGNAL(triggered()), SLOT(onPrintClicked()));

	tbar->addSeparator();

	act = initAction(staticActData.find);
	tbar->addAction(act);
	connect(act, SIGNAL(triggered()), SLOT(onFindTriggered()));
	QToolButton *findButton=
		dynamic_cast<QToolButton*>(tbar->widgetForAction(act));
	findButton->setPopupMode(QToolButton::InstantPopup);

	act = initAction(staticActData.replace);
	//tbar->addAction(act);
	connect(act, SIGNAL(triggered()), SLOT(onReplaceTriggered()));
	findButton->addAction(act);

	tbar->addSeparator();

	act = initAction(staticActData.trash);
	tbar->addAction(act);
	connect(act, SIGNAL(triggered()), SLOT(onTrashClicked()));

	connect(ui->noteEdit, SIGNAL(textChanged()), SLOT(textChanged()));

	ui->noteEdit->setText(""); // to force update event

	connect(ui->noteEdit, SIGNAL(focusLost()), SLOT(save()));
	connect(ui->noteEdit, SIGNAL(focusReceived()), SIGNAL(invalidated()), Qt::QueuedConnection);

}

NoteWidget::~NoteWidget()
{
	_autosaveTimer.stop();
	delete ui;
}

QAction* NoteWidget::initAction(const ActData &actData)
{
	QAction *act = new QAction(QIcon(actData.icon), tr(actData.text), this);
	act->setToolTip(actData.toolTip);
	act->setShortcut(QKeySequence(QLatin1String(actData.shortcut)));
	act->setShortcutContext(Qt::WindowShortcut);
	return act;
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

void NoteWidget::onFindTriggered()
{
	if (findBar->mode() == TypeAheadFindBar::Find) {
		findBar->toggleVisibility();
	} else {
		findBar->setMode(TypeAheadFindBar::Find);
		findBar->open();
	}
}


void NoteWidget::onReplaceTriggered()
{
	if (findBar->mode() == TypeAheadFindBar::Replace) {
		findBar->toggleVisibility();
	} else {
		findBar->setMode(TypeAheadFindBar::Replace);
		findBar->open();
	}
}

void NoteWidget::save()
{
	 if (_changed) { _changed = false; emit saveRequested(); _lastChangeElapsed.restart(); }
}

void NoteWidget::autosave()
{
	if (!text().isEmpty() && _changed) {
		save();
	} else {
		_autosaveTimer.stop(); // stop until next text change
	}
}

void NoteWidget::textChanged()
{
	_changed = true;
	if (!_autosaveTimer.isActive()) {
		_autosaveTimer.start();
		_lastChangeElapsed.restart();
	}
	QTextDocument *doc = ui->noteEdit->document();
	QTextBlock firstBlock = doc->begin();

	QString firstLine = firstBlock.text();
	if (firstLine != _firstLine || firstLine.isEmpty()) {
		_firstLine = firstLine;
		emit firstLineChanged();
	}
}

void NoteWidget::setText(QString text)
{
	ui->noteEdit->setPlainText(text);
	_changed = false; // mark as unchanged since its not user input.
	_autosaveTimer.stop(); // timer not required atm
	_lastChangeElapsed.restart();
}

QString NoteWidget::text()
{
	return ui->noteEdit->toPlainText().trimmed();
}

NoteDialogEdit* NoteWidget::editWidget() const
{
	return ui->noteEdit;
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

void NoteWidget::onCopyClicked()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(text());
}

void NoteWidget::onPrintClicked()
{
	QPrinter printer;
	if (!text().isEmpty()) {
		ui->noteEdit->print(&printer);
	}
}

void NoteWidget::onSaveClicked()
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

void NoteWidget::onTrashClicked()
{
	QSettings s;
	if (!text().isEmpty() && s.value("ui.ask-on-delete", true).toBool() &&
		QMessageBox::question(0, tr("Deletion confirmation"),
							  tr("Are you sure want to delete this note?"),
							  QMessageBox::Yes | QMessageBox::No) !=
		QMessageBox::Yes) {
		return;
	}
	_changed = false;
	_trashRequested  = true;
	emit trashRequested();
}

} // namespace QtNote
