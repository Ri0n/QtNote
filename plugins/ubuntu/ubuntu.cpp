/*
QtNote - Simple note-taking application
Copyright (C) 2010 Sergei Ilinykh

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

#include <QProcess>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QWidget>
#include <QtPlugin>

#include "qtnote_config.h"
#include "ubuntu.h"
#include "ubuntutray.h"
#include "x11util.h"

namespace QtNote {

static const QLatin1String pluginId("ubuntu_de");

//------------------------------------------------------------
// UbuntuPlugin
//------------------------------------------------------------
UbuntuPlugin::UbuntuPlugin(QObject *parent) : QObject(parent), _tray(0) { }

int UbuntuPlugin::metadataVersion() const { return MetadataVerion; }

PluginMetadata UbuntuPlugin::metadata()
{
    PluginMetadata md;
    md.id          = pluginId;
    md.icon        = QIcon(":/icons/ubuntu-logo");
    md.name        = "Ubuntu Integration";
    md.description = tr("Integrtion with ubuntu-only features");
    md.author      = "Sergei Ilinykh <rion4ik@gmail.com>";
    md.version     = 0x010000;       // plugin's version 0xXXYYZZPP
    md.minVersion  = 0x020300;       // minimum compatible version of QtNote
    md.maxVersion  = QTNOTE_VERSION; // maximum compatible version of QtNote
    md.homepage    = QUrl("http://ri0n.github.io/QtNote");
    md.extra.insert("de", QStringList() << "ubuntu");
    return md;
}

void UbuntuPlugin::setHost(PluginHostInterface *host) { this->host = host; }

TrayImpl *UbuntuPlugin::initTray(Main *qtnote)
{
    _tray = new UbuntuTray(qtnote, this);
    return _tray;
}

void UbuntuPlugin::notifyError(const QString &msg)
{
    // TODO ue libnotify instead or what is ubuntu-way
    if (_tray) {
        _tray->sti->showMessage(tr("Error"), msg, QSystemTrayIcon::Warning);
    }
}

void UbuntuPlugin::activateWidget(QWidget *w)
{
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), SLOT(activator()));
    timer->setProperty("widget", QVariant::fromValue<QWidget *>(w));
    timer->start(100);
}

void UbuntuPlugin::activator()
{
    QTimer  *timer = (QTimer *)sender();
    QWidget *w     = sender()->property("widget").value<QWidget *>();

    /*w->activateWindow();
    w->raise();*/
    X11Util::forceActivateWindow(w->winId());
    timer->deleteLater();
}

} // namespace QtNote
