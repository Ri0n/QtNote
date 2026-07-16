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

#include <QGuiApplication>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QWidget>
#include <QWindow>
#include <QtPlugin>

#include "freedesktopnotifier.h"
#include "gnome.h"
#include "qtnote_config.h"
#ifdef QTNOTE_ENABLE_X11
#include "x11util.h"
#endif

namespace QtNote {

static const QLatin1String pluginId("gnome_de");
static const QLatin1String shellExtensionId("qtnote@ri0n.github.io");

//------------------------------------------------------------
// GnomePlugin
//------------------------------------------------------------
GnomePlugin::GnomePlugin(QObject *parent) : QObject(parent) { }

int GnomePlugin::metadataVersion() const { return MetadataVersion; }

PluginMetadata GnomePlugin::metadata()
{
    PluginMetadata md;
    md.id          = pluginId;
    md.icon        = QIcon(":/icons/gnome-logo");
    md.name        = "Gnome Integration";
    md.description = tr("Integrtion with gnome-only features");
    md.author      = "Sergei Ilinykh <rion4ik@gmail.com>";
    md.version     = 0x01000000;     // plugin's version 0xXXYYZZPP
    md.minVersion  = 0x020300;       // minimum compatible version of QtNote
    md.maxVersion  = QTNOTE_VERSION; // maximum compatible version of QtNote
    md.homepage    = QUrl("http://ri0n.github.io/QtNote");
    md.extra.insert("de", QStringList() << "gnome");
    md.extra.insert("externalTray", true);
    return md;
}

void GnomePlugin::setHost(PluginHostInterface *host)
{
    this->host = host;
    QTimer::singleShot(0, this, &GnomePlugin::askEnableShellExtension);
}

void GnomePlugin::notifyError(const QString &msg)
{
    if (FreedesktopNotifier::notifyError(tr("Error"), msg))
        return;

    QMessageBox::warning(nullptr, tr("Error"), msg);
}

void GnomePlugin::askEnableShellExtension()
{
    if (!isShellExtensionInstalled())
        return;
    shellExtensionEnabled = isShellExtensionEnabled();
    if (shellExtensionEnabled)
        return;

    QSettings settings;
    settings.beginGroup(QLatin1String("gnomeintegration"));
    if (settings.value(QLatin1String("shellExtensionEnableAsked"), false).toBool())
        return;
    settings.setValue(QLatin1String("shellExtensionEnableAsked"), true);

    auto result = QMessageBox::question(nullptr, tr("Enable QtNote GNOME Extension"),
                                        tr("QtNote can enable a native GNOME Shell indicator. "
                                           "It provides Wayland-friendly access to recent notes."),
                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (result == QMessageBox::Yes) {
        shellExtensionEnabled = enableShellExtension();
        if (!shellExtensionEnabled) {
            QMessageBox::warning(nullptr, tr("Enable QtNote GNOME Extension"),
                                 tr("Failed to enable the QtNote GNOME Shell extension. "
                                    "You may need to log out and log back in before enabling it."));
        }
    }
}

bool GnomePlugin::isShellExtensionInstalled() const
{
    return !QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                      QLatin1String("gnome-shell/extensions/") + shellExtensionId
                                          + QLatin1String("/metadata.json"),
                                      QStandardPaths::LocateFile)
                .isEmpty();
}

bool GnomePlugin::isShellExtensionEnabled() const
{
    const QString executable = QStandardPaths::findExecutable(QLatin1String("gnome-extensions"));
    if (executable.isEmpty())
        return false;

    QProcess process;
    process.start(executable, { QLatin1String("info"), shellExtensionId });
    if (!process.waitForFinished(3000))
        return false;

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput()).toLower();
    // GNOME Shell 45+ reports an enabled, running extension as ACTIVE.
    // Older gnome-extensions versions used ENABLED for the same state.
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0
        && (output.contains(QLatin1String("state: active")) || output.contains(QLatin1String("state: enabled")));
}

bool GnomePlugin::enableShellExtension() const
{
    const QString executable = QStandardPaths::findExecutable(QLatin1String("gnome-extensions"));
    if (executable.isEmpty())
        return false;

    QProcess process;
    process.start(executable, { QLatin1String("enable"), shellExtensionId });
    if (!process.waitForFinished(5000))
        return false;

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

bool GnomePlugin::geometryExtensionAvailable()
{
    if (QGuiApplication::platformName() != QLatin1String("wayland"))
        return false;
    return geometryBridgeReady;
}

WindowGeometryRestoreResult GnomePlugin::restoreWindowGeometry(QWidget *, const QString &key)
{
    if (!geometryExtensionAvailable())
        return WindowGeometryRestoreResult::Unsupported;
    if (!pendingWindowGeometryKeys.contains(key))
        pendingWindowGeometryKeys.enqueue(key);
    return WindowGeometryRestoreResult::Pending;
}

bool GnomePlugin::saveWindowGeometry(QWidget *, const QString &) { return geometryExtensionAvailable(); }

bool GnomePlugin::removeWindowGeometry(const QString &) { return geometryExtensionAvailable(); }

QString GnomePlugin::takePendingWindowGeometryKey()
{
    return pendingWindowGeometryKeys.isEmpty() ? QString() : pendingWindowGeometryKeys.dequeue();
}

void GnomePlugin::windowGeometryBridgeReady() { geometryBridgeReady = true; }

void GnomePlugin::activateWidget(QWidget *w)
{
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), SLOT(activator()));
    timer->setProperty("widget", QVariant::fromValue<QWidget *>(w));
    timer->start(100);
}

void GnomePlugin::activator()
{
    QTimer  *timer = (QTimer *)sender();
    QWidget *w     = sender()->property("widget").value<QWidget *>();

    w->showNormal();
    w->raise();
    w->activateWindow();
    if (auto *window = w->windowHandle())
        window->requestActivate();
#ifdef QTNOTE_ENABLE_X11
    if (QGuiApplication::platformName() == QLatin1String("xcb"))
        X11Util::forceActivateWindow(w->winId());
#endif
    timer->deleteLater();
}

} // namespace QtNote
