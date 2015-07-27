#include <KStatusNotifierItem>
#include <KWindowSystem>
#ifndef USE_KDE5
#include <KAction>
#else
#include <QAction>
#include <KGlobalAccel>
#endif
#include <KNotification>
#include <QWidget>
#include <QtPlugin>

#include "kdeintegration.h"
#include "kdeintegrationtray.h"


namespace QtNote {

static const QLatin1String pluginId("kde_de");

//------------------------------------------------------------
// KDEIntegration
//------------------------------------------------------------
KDEIntegration::KDEIntegration(QObject *parent) :
    QObject(parent)
{
}

int KDEIntegration::metadataVersion() const
{
    return MetadataVerion;
}

PluginMetadata KDEIntegration::metadata()
{
    PluginMetadata md;
    md.id = pluginId;
    md.icon = QIcon(":/icons/kde-logo");
    md.name = "KDE Integration";
    md.description = tr("Provide native look and feel for KDE users");
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

void KDEIntegration::notifyError(const QString &msg)
{
    KNotification *n = KNotification::event(KNotification::Error, tr("Error"), msg, QPixmap(":/svg/qtnote"));
    n->sendEvent();
}

void KDEIntegration::activateWidget(QWidget *w)
{
    KWindowSystem::forceActiveWindow(w->winId(), 0); // just activateWindow doesn't work when started from tray
}

bool KDEIntegration::registerGlobalShortcut(const QString &id, const QKeySequence &key, QObject *receiver, const char *slot)
{
#ifdef USE_KDE5
    QAction *act = _shortcuts.value(id);
    if (!act) {
        act = new QAction(tr("New note from selection"), this);
        act->setObjectName(id);
        _shortcuts.insert(id, act);
    }
    KGlobalAccel::setGlobalShortcut(act, key);
#else
    //KGlobalAccel::setGlobalShortcut()
    KAction *act = _shortcuts.value(id);
    if (!act) {
        act = new KAction(tr("New note from selection"), this);
        act->setObjectName(id);
        _shortcuts.insert(id, act);
    }
    act->setGlobalShortcut(KShortcut(key));
#endif
    connect(act, SIGNAL(triggered()), receiver, slot, Qt::UniqueConnection);
    return true;
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(kdeintegration, QtNote::KDEIntegration)
#endif
