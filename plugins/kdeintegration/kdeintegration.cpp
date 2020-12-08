#include <KGlobalAccel>
#include <KNotification>
#include <KStatusNotifierItem>
#include <KWindowSystem>
#include <QAction>
#include <QWidget>
#include <QtPlugin>

#include "kdeintegration.h"
#include "kdeintegrationtray.h"
#include "pluginhostinterface.h"
#include "qtnote_config.h"

namespace QtNote {

static const QLatin1String pluginId("kde_de");

//------------------------------------------------------------
// KDEIntegration
//------------------------------------------------------------
KDEIntegration::KDEIntegration(QObject *parent) : QObject(parent) { }

int KDEIntegration::metadataVersion() const { return MetadataVerion; }

PluginMetadata KDEIntegration::metadata()
{
    PluginMetadata md;
    md.id          = pluginId;
    md.icon        = QIcon(":/icons/kde-logo");
    md.name        = "KDE Integration";
    md.description = tr("Provide native look and feel for KDE users");
    md.author      = "Sergey Il'inykh <rion4ik@gmail.com>";
    md.version     = 0x010000;       // plugin's version 0xXXYYZZPP
    md.minVersion  = 0x020300;       // minimum compatible version of QtNote
    md.maxVersion  = QTNOTE_VERSION; // maximum compatible version of QtNote
    md.homepage    = QUrl("http://ri0n.github.io/QtNote");
    md.extra.insert("de",
                    QStringList() << "KDE-4"
                                  << "kde-plasma"
                                  << "/usr/share/xsessions/plasma"
                                  << "plasma"
                                  << "/usr/share/xsessions/plasma5");
    return md;
}

void KDEIntegration::setHost(PluginHostInterface *) { }

TrayImpl *KDEIntegration::initTray(Main *qtnote) { return new KDEIntegrationTray(qtnote, this); }

void KDEIntegration::notifyError(const QString &msg)
{
    KNotification *n = KNotification::event(KNotification::Error, tr("Error"), msg, QPixmap(":/svg/qtnote"));
    n->sendEvent();
}

void KDEIntegration::activateWidget(QWidget *w)
{
    KWindowSystem::forceActiveWindow(w->winId(), 0); // just activateWindow doesn't work when started from tray
}

bool KDEIntegration::registerGlobalShortcut(const QString &id, const QKeySequence &key, QAction *action)
{
    QAction *act = _shortcuts.value(id);
    if (!act) {
        act = new QAction(action->text(), this);
        act->setObjectName(id);
        _shortcuts.insert(id, act);
    }
    KGlobalAccel::setGlobalShortcut(act, key);
    connect(act, SIGNAL(triggered()), action, SLOT(trigger()), Qt::UniqueConnection);
    return true;
}

bool KDEIntegration::updateGlobalShortcut(const QString &id, const QKeySequence &key)
{
    auto act = _shortcuts.value(id);
    if (!act) {
        return false;
    }
    KGlobalAccel::setGlobalShortcut(act, key);
    return true;
}

void KDEIntegration::setGlobalShortcutEnabled(const QString &id, bool enabled)
{
    auto act = _shortcuts.value(id);
    if (!act) {
        return;
    }
    act->setEnabled(enabled);
}

} // namespace QtNote
