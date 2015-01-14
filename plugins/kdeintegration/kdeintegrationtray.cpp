#include <KDE/KMenu>
#include <KDE/KNotification>
#include <KDE/KStatusNotifierItem>

#include "kdeintegrationtray.h"

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

	contextMenu->addAction(QIcon(":/icons/new"), tr("&New"), this, SIGNAL(newNoteTriggered()));
	contextMenu->addSeparator();
	contextMenu->addAction(QIcon(":/icons/manager"), tr("&Note Manager"), this, SIGNAL(noteManagerTriggered()));
	contextMenu->addAction(QIcon(":/icons/options"), tr("&Options"), this, SIGNAL(optionsTriggered()));
	contextMenu->addAction(QIcon(":/icons/trayicon"), tr("&About"), this, SIGNAL(aboutTriggered()));
	contextMenu->addSeparator();
}

void KDEIntegrationTray::notifyError(const QString &message)
{
	KNotification *n = KNotification::event(KNotification::Error, tr("Error"), message);
	n->sendEvent();
}

} // namespace QtNote
