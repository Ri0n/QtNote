#include "mainwidget.h"
//#include <QDir>
//#include <QFileInfoList>
#include <QtGui/QApplication>
//#include <QDesktopServices>
//#include <QUuid>
#include "notemanager.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
	actQuit = new QAction(QIcon(":/icons/exit"), "&Quit", this);
	actNew = new QAction(QIcon(":/icons/new"), "&New", this);

	contextMenu = new QMenu(this);
	contextMenu->addAction(actNew);
	contextMenu->addSeparator();
	contextMenu->addAction(actQuit);


	tray = new QSystemTrayIcon(this);
	tray->setIcon(QIcon(":/icons/trayicon"));
	tray->show();
	tray->setContextMenu(contextMenu);

	foreach (NoteStorage *storage, NoteManager::instance()->storages()) {
		noteDialogs[storage->systemName()] = QMap<QString, NoteDialog *>();
	}

	connect(actQuit, SIGNAL(triggered()), this, SLOT(exitQtNote()));
	connect(actNew, SIGNAL(triggered()), this, SLOT(createNewNote()));
	connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(showNoteList(QSystemTrayIcon::ActivationReason)));
}

Widget::~Widget()
{
}

void Widget::exitQtNote()
{
	QApplication::quit();
}

void Widget::showNoteDialog(const QString &storageId, const QString &noteId)
{
	Note *note = NoteManager::instance()->loadNote(storageId, noteId);
	NoteDialog *dlg;
	if (note) {
		dlg = noteDialogs[storageId].value(noteId);
		if (!dlg) {
			dlg = noteDialogs[storageId][noteId] = new NoteDialog(this);

			dlg->setText(note->text());
			dlg->setWindowIcon(QIcon(":/icons/pen"));
			dlg->setWindowTitle(note->title());
			dlg->show();
			connect(dlg, SIGNAL(finished(int)), this, SLOT(onCloseNote(int)));
		}
		noteDialogs[storageId][noteId]->raise();
		noteDialogs[storageId][noteId]->activateWindow();
	}
}

void Widget::showNoteList(QSystemTrayIcon::ActivationReason reason)
{
	if (reason != QSystemTrayIcon::Trigger) {
		return;
	}
//	if (notes.isEmpty()) {
//		QFileInfoList files = QDir(QDir::home().path()+"/.tomboy").entryInfoList(QStringList("*.note"),
//				  QDir::Files | QDir::NoDotAndDotDot);
//		foreach (QFileInfo fi, files) {
//			notes.append(new TomboyNote(fi.canonicalFilePath()));
//		}
//	}
	QMenu menu(this);
	menu.addAction(actNew);
	menu.addSeparator();
	QList<NoteListItem> notes = NoteManager::instance()->noteList();
	for (int i=0; i<notes.count(); i++) {
		menu.addAction(notes[i].title)->setData(i);;
	}
	const QPoint menuPos = tray->geometry().bottomLeft();
	QAction *act = menu.exec(tray->geometry().bottomLeft(), actNew);
	if (act && act != actNew) {
		NoteListItem &note = notes[act->data().toInt()];
		showNoteDialog(note.storageId, note.id);
	}
}

void Widget::createNewNote()
{
//	TomboyNote *note = new TomboyNote();
//	notes.prepend(note);
//	QString uid = QUuid::createUuid ().toString();
//	uid = uid.mid(1, uid.length()-2);
//	note->setFile(QDir::home().path()+"/.tomboy/"+uid+".note");
//	note->showDialog();
}
