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

#include <QAction>
#include <QKeySequence>
#include <QLoggingCategory>
#include <QProcess>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QWidget>
#include <QtPlugin>

#include "gnome.h"
#include "gnometray.h"
#include "qtnote_config.h"
#include "shortcutsmanager.h"
#include "x11util.h"

namespace QtNote {

static const QLatin1String pluginId("gnome_de");
static const QLatin1String noteFromSelectionShortcutId(ShortcutsManager::SKNoteFromSelection);
static const QLatin1String gnomeShellShortcutSchema("org.gnome.shell.extensions.qtnote");

Q_LOGGING_CATEGORY(logGnomeIntegration, "qtnote.gnomeintegration")

namespace {

    QString acceleratorFromKeySequence(const QKeySequence &key)
    {
        if (key.isEmpty())
            return {};

        const auto parts  = key.toString(QKeySequence::PortableText).split(QLatin1Char(','));
        const auto tokens = parts.constFirst().split(QLatin1Char('+'), Qt::SkipEmptyParts);
        if (tokens.isEmpty())
            return {};

        QString accelerator;
        for (int i = 0; i < tokens.size() - 1; ++i) {
            const auto token = tokens.at(i);
            if (token == QLatin1String("Ctrl"))
                accelerator += QLatin1String("<Ctrl>");
            else if (token == QLatin1String("Alt"))
                accelerator += QLatin1String("<Alt>");
            else if (token == QLatin1String("Shift"))
                accelerator += QLatin1String("<Shift>");
            else if (token == QLatin1String("Meta"))
                accelerator += QLatin1String("<Super>");
            else
                accelerator += QLatin1Char('<') + token + QLatin1Char('>');
        }
        accelerator += tokens.constLast();
        return accelerator;
    }

    QString shellExtensionSchemasDir()
    {
#ifdef QTNOTE_GNOME_SHELL_EXTENSION_DIR
        return QStringLiteral(QTNOTE_GNOME_SHELL_EXTENSION_DIR "/schemas");
#else
        return {};
#endif
    }

} // namespace

//------------------------------------------------------------
// GnomePlugin
//------------------------------------------------------------
GnomePlugin::GnomePlugin(QObject *parent) : QObject(parent), _tray(0) { }

int GnomePlugin::metadataVersion() const { return MetadataVersion; }

PluginMetadata GnomePlugin::metadata()
{
    PluginMetadata md;
    md.id          = pluginId;
    md.icon        = QIcon(":/icons/gnome-logo");
    md.name        = "Gnome Integration";
    md.description = tr("Integrtion with gnome-only features");
    md.author      = "Sergei Ilinykh <rion4ik@gmail.com>";
    md.version     = 0x010000;       // plugin's version 0xXXYYZZPP
    md.minVersion  = 0x020300;       // minimum compatible version of QtNote
    md.maxVersion  = QTNOTE_VERSION; // maximum compatible version of QtNote
    md.homepage    = QUrl("http://ri0n.github.io/QtNote");
    md.extra.insert("de", QStringList() << "gnome");
    return md;
}

void GnomePlugin::setHost(PluginHostInterface *host) { this->host = host; }

TrayImpl *GnomePlugin::initTray(Main *qtnote)
{
    _tray = new GnomeTray(qtnote, this);
    return _tray;
}

void GnomePlugin::notifyError(const QString &msg)
{
    // TODO ue libnotify instead or what is gnome-way
    if (_tray) {
        _tray->sti->showMessage(tr("Error"), msg, QSystemTrayIcon::Warning);
    }
}

void GnomePlugin::activateWidget(QWidget *w)
{
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), SLOT(activator()));
    timer->setProperty("widget", QVariant::fromValue<QWidget *>(w));
    timer->start(100);
}

bool GnomePlugin::registerGlobalShortcut(const QString &id, const QKeySequence &key, QAction *action)
{
    Q_UNUSED(action)

    if (id != noteFromSelectionShortcutId) {
        qCWarning(logGnomeIntegration) << "unsupported global shortcut id" << id;
        return false;
    }

    _shortcuts.insert(id, key);
    _shortcutEnabled.insert(id, true);
    if (!setShellShortcut(id, key))
        qCWarning(logGnomeIntegration)
            << "global shortcut will use the GNOME Shell schema default until settings are updated"
            << "id=" << id;
    return true;
}

bool GnomePlugin::updateGlobalShortcut(const QString &id, const QKeySequence &key)
{
    if (id != noteFromSelectionShortcutId) {
        qCWarning(logGnomeIntegration) << "unsupported global shortcut id" << id;
        return false;
    }

    _shortcuts.insert(id, key);
    if (!_shortcutEnabled.value(id, true))
        return true;

    return setShellShortcut(id, key);
}

void GnomePlugin::setGlobalShortcutEnabled(const QString &id, bool enabled)
{
    if (id != noteFromSelectionShortcutId) {
        qCWarning(logGnomeIntegration) << "unsupported global shortcut id" << id << "enabled=" << enabled;
        return;
    }

    _shortcutEnabled.insert(id, enabled);
    setShellShortcut(id, enabled ? _shortcuts.value(id) : QKeySequence());
}

bool GnomePlugin::setShellShortcut(const QString &id, const QKeySequence &key)
{
    const QString schemasDir = shellExtensionSchemasDir();
    if (schemasDir.isEmpty()) {
        qCWarning(logGnomeIntegration) << "GNOME Shell extension schemas directory is not configured";
        return false;
    }

    const QString gsettings = QStandardPaths::findExecutable(QStringLiteral("gsettings"));
    if (gsettings.isEmpty()) {
        qCWarning(logGnomeIntegration) << "gsettings executable was not found";
        return false;
    }

    const QString accelerator = acceleratorFromKeySequence(key);
    const QString value = accelerator.isEmpty() ? QStringLiteral("[]") : QStringLiteral("['%1']").arg(accelerator);

    QProcess proc;
    proc.start(gsettings,
               {
                   QStringLiteral("--schemadir"),
                   schemasDir,
                   QStringLiteral("set"),
                   gnomeShellShortcutSchema,
                   id,
                   value,
               });
    if (!proc.waitForStarted()) {
        qCWarning(logGnomeIntegration) << "failed to start gsettings:" << proc.errorString();
        return false;
    }
    if (!proc.waitForFinished(3000)) {
        proc.kill();
        proc.waitForFinished();
        qCWarning(logGnomeIntegration) << "gsettings timed out while updating shortcut" << id;
        return false;
    }
    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        qCWarning(logGnomeIntegration).noquote() << "failed to update GNOME Shell shortcut"
                                                 << "id=" << id << "value=" << value << "exitCode=" << proc.exitCode()
                                                 << "stderr=" << QString::fromUtf8(proc.readAllStandardError());
        return false;
    }

    return true;
}

void GnomePlugin::activator()
{
    QTimer  *timer = (QTimer *)sender();
    QWidget *w     = sender()->property("widget").value<QWidget *>();

    w->activateWindow();
    w->raise();
    // X11Util::forceActivateWindow(w->winId());
    timer->deleteLater();
}

} // namespace QtNote
