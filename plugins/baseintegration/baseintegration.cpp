#include <QWidget>
#include <QtPlugin>
#include <QAction>

#include "baseintegration.h"
#include "baseintegrationtray.h"
#include "qxtglobalshortcut.h"

namespace QtNote {

static const QLatin1String pluginId("base_de");

//******************************************
// BaseIntegration
//******************************************
BaseIntegration::BaseIntegration(QObject *parent) :
    QObject(parent),
    tray(0)
{
}

int BaseIntegration::metadataVersion() const
{
    return MetadataVerion;
}

PluginMetadata BaseIntegration::metadata()
{
    PluginMetadata md;
    md.id = pluginId;
    md.icon = QIcon(":/icons/logo");
    md.name = "Base Integration";
    md.description = tr("Provides fallback desktop environment integration");
    md.author = "Sergey Il'inykh <rion4ik@gmail.com>";
    md.version = 0x010100;    // plugin's version 0xXXYYZZPP
    md.minVersion = 0x030002; // minimum compatible version of QtNote
    md.maxVersion = QTNOTE_VERSION; // maximum compatible version of QtNote
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

bool BaseIntegration::registerGlobalShortcut(const QString &id, const QKeySequence &key, QAction *action)
{
    if (!_shortcuts.contains(id)) {
        auto gs = new QxtGlobalShortcut(key, this);
        _shortcuts.insert(id, gs);
        connect(gs, SIGNAL(activated()), action, SLOT(trigger()));
        return true;
    }
    return false;
}

bool BaseIntegration::updateGlobalShortcut(const QString &id, const QKeySequence &key)
{
    auto sc = _shortcuts.value(id);
    if (sc) {
        return sc->setShortcut(key);
    }
    return false;
}

void BaseIntegration::setGlobalShortcutEnabled(const QString &id, bool enabled)
{
    auto sc = _shortcuts.value(id);
    if (sc) {
        sc->setEnabled(enabled);
    }
}

void BaseIntegration::notifyError(const QString &message)
{
    if (tray) {
        ((BaseIntegrationTray*)tray)->tray->showMessage(tr("Error"), message, QSystemTrayIcon::Warning, 5000);
    }
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(kdeintegration, QtNote::BaseIntegration)
#endif
