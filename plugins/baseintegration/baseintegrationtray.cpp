#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QStyle>
#include <QApplication>
#include <QDesktopWidget>

#include "baseintegrationtray.h"
#include "utils.h"
#include "qtnote.h"

namespace QtNote {

BaseIntegrationTray::BaseIntegrationTray(Main *qtnote, QObject *parent) :
	TrayImpl(parent),
	qtnote(qtnote)
{
	actQuit = new QAction(QIcon(":/icons/exit"), tr("&Quit"), this);
	actNew = new QAction(QIcon(":/icons/new"), tr("&New"), this);
	actAbout = new QAction(QIcon(":/icons/trayicon"), tr("&About"), this);
	actOptions = new QAction(QIcon(":/icons/options"), tr("&Options"), this);
	actManager = new QAction(QIcon(":/icons/manager"), tr("&Note Manager"), this);

	contextMenu = new QMenu;
	contextMenu->addAction(actNew);
	contextMenu->addSeparator();
	contextMenu->addAction(actManager);
	contextMenu->addAction(actOptions);
	contextMenu->addAction(actAbout);
	contextMenu->addSeparator();
	contextMenu->addAction(actQuit);

	tray = new QSystemTrayIcon(this);
	tray->setIcon(QIcon(":/icons/trayicon"));
	tray->show();
	tray->setContextMenu(contextMenu);

	connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
			SLOT(showNoteList(QSystemTrayIcon::ActivationReason)));

	connect(actQuit, SIGNAL(triggered()), SIGNAL(exitTriggered()));
	connect(actNew, SIGNAL(triggered()), SIGNAL(newNoteTriggered()));
	connect(actManager, SIGNAL(triggered()), SIGNAL(noteManagerTriggered()));
	connect(actOptions, SIGNAL(triggered()), SIGNAL(optionsTriggered()));
	connect(actAbout, SIGNAL(triggered()), SIGNAL(aboutTriggered()));
}

void BaseIntegrationTray::notifyError(const QString &message)
{
	tray->showMessage(tr("Error"), message, QSystemTrayIcon::Warning, 5000);
}

void BaseIntegrationTray::setNoteList(QList<NoteListItem>)
{

}

void BaseIntegrationTray::showNoteList(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::MiddleClick || reason == QSystemTrayIcon::DoubleClick) {
		emit newNoteTriggered();
		return;
	}
	if (reason != QSystemTrayIcon::Trigger) {
		return;
	}
	QMenu menu;
	menu.addAction(actNew);
	menu.addSeparator();
	QSettings s;
	QList<NoteListItem> notes = NoteManager::instance()->noteList(
								s.value("ui.menu-notes-amount", 15).toInt());
	for (int i=0; i<notes.count(); i++) {
		menu.addAction(menu.style()->standardIcon(
			QStyle::SP_MessageBoxInformation),
			Utils::cuttedDots(notes[i].title, 48).replace('&', "&&")
		)->setData(i);
	}
	menu.show();
	qtnote->activateWidget(&menu);
	QRect dr = QApplication::desktop()->availableGeometry(QCursor::pos());
	QRect ir = tray->geometry();
	QRect mr = menu.geometry();
	if (ir.isEmpty()) { // O_O but with kde this happens...
		ir = QRect(QCursor::pos() - QPoint(8,8), QSize(16,16));
	}
	mr.setSize(menu.sizeHint());
	if (ir.left() < dr.width()/2) {
		if (ir.top() < dr.height()/2) {
			mr.moveTopLeft(ir.bottomLeft());
		} else {
			mr.moveBottomLeft(ir.topLeft());
		}
	} else {
		if (ir.top() < dr.height()/2) {
			mr.moveTopRight(ir.bottomRight());
		} else {
			mr.moveBottomRight(ir.topRight());
		}
	}
	// and now align to available desktop geometry
	if (mr.right() > dr.right()) {
		mr.moveRight(dr.right());
	}
	if (mr.bottom() > dr.bottom()) {
		mr.moveBottom(dr.bottom());
	}
	if (mr.left() < dr.left()) {
		mr.moveLeft(dr.left());
	}
	if (mr.top() < dr.top()) {
		mr.moveTop(dr.top());
	}
	QAction *act = menu.exec(mr.topLeft());

	if (act && act != actNew) {
		NoteListItem &note = notes[act->data().toInt()];
		emit showNoteTriggered(note.storageId, note.id);
	}
}

} // namespace QtNote
