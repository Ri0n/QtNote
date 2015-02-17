#include <KDE/KStatusNotifierItem>
#include <KDE/KWindowSystem>
#include <KDE/KAction>
#include <QWidget>
#include <QtPlugin>

#include "kdeintegration.h"
#include "kdeintegrationtray.h"

namespace QtNote {


//------------------------------------------------------------
// KDEIntegration
//------------------------------------------------------------
KDEIntegration::KDEIntegration(QObject *parent) :
	QObject(parent)
{
}

PluginMetadata KDEIntegration::metadata()
{
	PluginMetadata md;
	md.icon = QIcon(":/icons/kde-logo");
	md.name = "KDE Integration";
	md.description = "Provide native look and feel for KDE users";
	md.author = "Sergey Il'inykh <rion4ik@gmail.com>";
	md.version = 0x010000;	// plugin's version 0xXXYYZZPP
	md.minVersion = 0x020300; // minimum compatible version of QtNote
	md.maxVersion = 0x030000; // maximum compatible version of QtNote
	md.homepage = QUrl("http://ri0n.github.io/QtNote");
	md.extra.insert("de", QStringList() << "KDE-4" << "kde-plasma");
	return md;
}

TrayImpl *KDEIntegration::initTray(Main *qtnote)
{
	return new KDEIntegrationTray(qtnote, this);
}

void KDEIntegration::activateWidget(QWidget *w)
{
	KWindowSystem::forceActiveWindow(w->winId(), 0); // just activateWindow doesn't work when started from tray
}

bool KDEIntegration::registerGlobalShortcut(const QString &id, const QKeySequence &key, QObject *receiver, const char *slot)
{
	KAction *act = _shortcuts.value(id);
	if (!act) {
		act = new KAction(tr("New note from selection"), this);
		act->setObjectName(id);
		_shortcuts.insert(id, act);
	}
	act->setGlobalShortcut(KShortcut(key));
	connect(act, SIGNAL(triggered()), receiver, slot, Qt::UniqueConnection);
	return true;
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(kdeintegration, QtNote::KDEIntegration)
#endif
