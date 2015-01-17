#include <KDE/KMenu>
#include <KDE/KNotification>
#include <KDE/KStatusNotifierItem>
#include <QSettings>
#include <QStyle>
#include <QApplication>
#include <QDesktopWidget>

#include "kdeintegrationtray.h"
#include "notemanager.h"
#include "utils.h"
#include "qtnote.h"

namespace QtNote {

KDEIntegrationTray::KDEIntegrationTray(Main *qtnote, QObject *parent) :
	TrayImpl(parent),
	qtnote(qtnote)
{
	sni = new KStatusNotifierItem("qtnote", this);
	sni->setIconByName("qtnote"); // TODO review dev mode when no icon in /usr/share
	sni->setStatus(KStatusNotifierItem::Active);
	sni->setTitle("Notes");

	KMenu *contextMenu = new KMenu;
	sni->setContextMenu(contextMenu);

	actNew = new QAction(QIcon(":/icons/new"), tr("&New"), this);
	contextMenu->addAction(actNew);
	contextMenu->addSeparator();
	contextMenu->addAction(QIcon(":/icons/manager"), tr("&Note Manager"), this, SIGNAL(noteManagerTriggered()));
	contextMenu->addAction(QIcon(":/icons/options"), tr("&Options"), this, SIGNAL(optionsTriggered()));
	contextMenu->addAction(QIcon(":/icons/trayicon"), tr("&About"), this, SIGNAL(aboutTriggered()));

	connect(actNew, SIGNAL(triggered()), SIGNAL(newNoteTriggered()));
	connect(sni, SIGNAL(activateRequested(bool,QPoint)), SLOT(showNotes(bool,QPoint)));
	connect(sni, SIGNAL(secondaryActivateRequested(QPoint)), SIGNAL(newNoteTriggered()));
}

void KDEIntegrationTray::notifyError(const QString &message)
{
	KNotification *n = KNotification::event(KNotification::Error, tr("Error"), message);
	n->sendEvent();
}

void KDEIntegrationTray::showNotes(bool active, const QPoint &pos)
{
	Q_UNUSED(active)
	KMenu menu;
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
	QRect mr = menu.geometry();
	mr.setSize(menu.sizeHint());
	QPoint mp = pos - QPoint(mr.width() / 2, 0);
	if (pos.y() < dr.height()/2) { // icon at top-left
		mr.moveTopLeft(mp);
	} else { // icons at bottom-left
		mr.moveBottomLeft(mp);
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
