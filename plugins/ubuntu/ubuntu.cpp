/*
QtNote - Simple note-taking application
Copyright (C) 2010 Ili'nykh Sergey

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Contacts:
E-Mail: rion4ik@gmail.com XMPP: rion@jabber.ru
*/

#include <QWidget>
#include <QtPlugin>
#include <QProcess>
#include <QTimer>

#include "ubuntu.h"
#include "ubuntutray.h"
#include "x11util.h"

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

void UbuntuPlugin::activateWidget(QWidget *w)
{
	QTimer *timer = new QTimer(this);
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), SLOT(activator()));
	timer->setProperty("widget", QVariant::fromValue<QWidget*>(w));
	timer->start(100);
}


void UbuntuPlugin::activator()
{
	QTimer *timer = (QTimer*)sender();
	QWidget *w = sender()->property("widget").value<QWidget*>();

	/*w->activateWindow();
	w->raise();*/
	X11Util::forceActivateWindow(w->winId());
	timer->deleteLater();
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(ubuntuplugin, QtNote::UbuntuPlugin)
#endif
