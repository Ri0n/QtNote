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

namespace QtNote {

QHash<QString,QAction*> NoteWidget::_actions;

NoteWidget::NoteWidget(const QString &storageId, const QString &noteId) :
	ui(new Ui::NoteWidget),
	_storageId(storageId),
	noteId_(noteId),
	_trashRequested(false)
{
	ui->setupUi(this);

	setFocusProxy(ui->noteEdit);
	ui->saveBtn->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

	QHBoxLayout *hb3a = new QHBoxLayout();
	findBar = new TypeAheadFindBar(ui->noteEdit, QString::null, this);
	hb3a->addWidget(findBar);
	ui->noteLayout->addLayout(hb3a);

	_autosaveTimer.setInterval(10000);
	_lastChangeElapsed.start();
	connect(&_autosaveTimer, SIGNAL(timeout()), SLOT(autosave()));
	//connect(parent, SIGNAL(destroyed()), SLOT(close()));
	
	QToolBar *tbar = new QToolBar(this);
	
	QAction *act = cloneAction("save");
	tbar->addAction(act);
	connect(act, SIGNAL(triggered()), SLOT(onSaveClicked()));

	act = cloneAction("copy");
	tbar->addAction(act);
	connect(act, SIGNAL(triggered()), SLOT(onSaveClicked()));

	act = cloneAction("print");
	tbar->addAction(act);
	connect(act, SIGNAL(triggered()), SLOT(onPrintClicked()));

	tbar->addSeparator();

	act = cloneAction("find");
	tbar->addAction(act);
	connect(act, SIGNAL(triggered()), findBar, SLOT(toggleVisibility()));

	tbar->addSeparator();

	act = cloneAction("delete");
	tbar->addAction(act);
	connect(act, SIGNAL(triggered()), SLOT(onDeleteClicked()));

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
void NoteWidget::initActions()
{
	struct ActionInfo {
		const char *name;
		const char *icon;
		const QString text;
		const QString toolTip;
		const char *shortcut;
	};


	ActionInfo actData[] = {
		{"save",  ":/save",  QObject::tr("Save"),  QObject::tr("Save note to file"), "Ctrl+S"},
		{"copy",  ":/copy",  QObject::tr("Copy"),  QObject::tr("Copy note to clipboard"), "Ctrl+Shift+C"},
		{"print", ":/print", QObject::tr("Print"), QObject::tr("Print note"), "Ctrl+P"},
		{"find",  ":/find",  QObject::tr("Find"),  QObject::tr("Find text in note"), "Ctrl+F"},
		{"trash", ":/trash", QObject::tr("Delete"), QObject::tr("Delete note"), "Ctrl+D"}
	};

	for (size_t i = 0; i < sizeof(actData); i++) {
		QAction *act = new QAction(QIcon(actData[i].icon), actData[i].text, qApp);
		act->setToolTip(actData[i].toolTip);
		act->setShortcut(QKeySequence(QLatin1String(actData[i].shortcut)));
		act->setShortcutContext(Qt::WindowShortcut);
		_actions.insert(actData[i].name, act);
	}
}

QAction* NoteWidget::cloneAction(const QString &name)
{
	QAction *act = _actions.value(name);
	QAction *newAct = new QAction(qApp);
	newAct->setText(act->text());
	newAct->setIcon(act->icon());
	newAct->setToolTip(act->toolTip());
	newAct->setShortcut(act->shortcut());
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
	emit trashRequested();
}

} // namespace QtNote
