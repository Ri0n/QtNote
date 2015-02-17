#include <QWidget>
#include <QtPlugin>

#include "baseintegration.h"
#include "baseintegrationtray.h"
#include "qxtglobalshortcut.h"

namespace QtNote {

//******************************************
// BaseIntegration
//******************************************
BaseIntegration::BaseIntegration(QObject *parent) :
	QObject(parent)
{
}

int BaseIntegration::metadataVersion() const
{
	return MetadataVerion;
}

PluginMetadata BaseIntegration::metadata()
{
	PluginMetadata md;
	md.icon = QIcon(":/icons/logo");
	md.name = "Base Integration";
	md.description = "Provides fallback desktop environment integration";
	md.author = "Sergey Il'inykh <rion4ik@gmail.com>";
	md.version = 0x010000;	// plugin's version 0xXXYYZZPP
	md.minVersion = 0x020301; // minimum compatible version of QtNote
	md.maxVersion = 0x030000; // maximum compatible version of QtNote
	md.homepage = QUrl("http://ri0n.github.io/QtNote");
	//md.extra.insert("de", QStringList() << "KDE-4");
	return md;
}

void BaseIntegration::activateWidget(QWidget *w)
{
	w->raise();
	w->activateWindow();
}

TrayImpl* BaseIntegration::initTray(Main *qtnote)
{
	tray = new BaseIntegrationTray(qtnote, this);
	return tray;
}

bool BaseIntegration::registerGlobalShortcut(const QString &id, const QKeySequence &key, QObject *receiver, const char *slot)
{
	// TODO remember id for future deregistration
	Q_UNUSED(id)
	QxtGlobalShortcut *gs = new QxtGlobalShortcut(key, this);
	connect(gs, SIGNAL(activated()), receiver, slot);
	return true;
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(kdeintegration, QtNote::BaseIntegration)
#endif
