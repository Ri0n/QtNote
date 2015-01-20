#include <QWidget>
#include <QtPlugin>

#include "ubuntu.h"
#include "ubuntutray.h"

namespace QtNote {


//------------------------------------------------------------
// UbuntuPlugin
//------------------------------------------------------------
UbuntuPlugin::UbuntuPlugin(QObject *parent) :
	QObject(parent)
{
}

PluginMetadata UbuntuPlugin::metadata()
{
	PluginMetadata md;
	md.pluginType = PluginMetadata::DEIntegration;
	md.icon = QIcon(":/icons/logo");
	md.name = "Ubuntu Integration";
	md.description = "Integrtion with ubuntu-only features";
	md.author = "Sergey Il'inykh <rion4ik@gmail.com>";
	md.version = 0x010000;	// plugin's version 0xXXYYZZPP
	md.minVersion = 0x020300; // minimum compatible version of QtNote
	md.maxVersion = 0x030000; // maximum compatible version of QtNote
	md.extra.insert("de", QStringList() << "ubuntu");
	return md;
}

bool UbuntuPlugin::init(Main *qtnote)
{
	Q_UNUSED(qtnote)
	return true;
}

TrayImpl *UbuntuPlugin::initTray(Main *qtnote)
{
	return new UbuntuTray(qtnote, this);
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(ubuntuplugin, QtNote::UbuntuPlugin)
#endif
