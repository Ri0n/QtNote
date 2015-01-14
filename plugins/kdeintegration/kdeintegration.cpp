#include <KDE/KStatusNotifierItem>
#include <KDE/KWindowSystem>
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
	md.pluginType = PluginMetadata::DEIntegration;
	md.icon = QIcon(":/icons/logo");
	md.name = "KDE Integration";
	md.description = "Provide native look and feel for KDE users";
	md.author = "Sergey Il'inykh <rion4ik@gmail.com>";
	md.version = 0x010000;	// plugin's version 0xXXYYZZPP
	md.minVersion = 0x020300; // minimum compatible version of QtNote
	md.maxVersion = 0x030000; // maximum compatible version of QtNote
	md.extra.insert("de", QStringList() << "KDE-4");
	return md;
}

bool KDEIntegration::init(Main *qtnote)
{
	Q_UNUSED(qtnote)
	return true;
}

TrayImpl *KDEIntegration::initTray(Main *qtnote)
{
	return new KDEIntegrationTray(qtnote, this);
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(kdeintegration, QtNote::KDEIntegration)
#endif
