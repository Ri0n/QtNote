#include "widget.h"
#include <QDir>
#include <QFileInfoList>
#include <QtGui/QApplication>
#include <QDesktopServices>
#include <QUuid>

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

void Widget::showNoteList(QSystemTrayIcon::ActivationReason reason)
{
	if (reason != QSystemTrayIcon::Trigger) {
		return;
	}
	if (notes.isEmpty()) {
		QFileInfoList files = QDir(QDir::home().path()+"/.tomboy").entryInfoList(QStringList("*.note"),
				  QDir::Files | QDir::NoDotAndDotDot);
		foreach (QFileInfo fi, files) {
			notes.append(new TomboyNote(fi.canonicalFilePath()));
		}
	}
	QList<QAction *> actions;
	actions.append(actNew);
	actions.append(new QAction(this));
	actions[1]->setSeparator(true);
	foreach (TomboyNote *note, notes) {
		actions.append(new QAction(note->title(), this));
	}
	const QPoint menuPos = tray->geometry().bottomLeft();
	QAction *act = QMenu::exec(actions, tray->geometry().bottomLeft()
							   , actNew, this);
	if (act && act != actNew) {
		TomboyNote *note = notes[actions.indexOf(act)-2];
		note->showDialog();
	}

	while (actions.size() > 1) {
		delete actions.takeLast();
		actions.removeLast();
	}
}

void Widget::createNewNote()
{
	TomboyNote *note = new TomboyNote();
	notes.prepend(note);
	QString uid = QUuid::createUuid ().toString();
	uid = uid.mid(1, uid.length()-2);
	note->setFile(QDir::home().path()+"/.tomboy/"+uid+".note");
	note->showDialog();
}
