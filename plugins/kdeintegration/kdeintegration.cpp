#include <KConfigGroup>
#include <KGlobalAccel>
#include <KNotification>
#include <KSharedConfig>
#include <KWindowConfig>
#include <KWindowSystem>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0) && !defined(OLD_K_FORCE_ACTIVATE)
#include <KX11Extras>
#endif

#include <QAction>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include <QWindow>
#include <QtPlugin>

#include "kdeintegration.h"
#include "kdeintegrationtray.h"
#include "pluginhostinterface.h"
#include "qtnote_config.h"

namespace QtNote {

Q_LOGGING_CATEGORY(logKdeIntegration, "qtnote.kdeintegration")

static const QLatin1String pluginId("kde_de");

//------------------------------------------------------------
// KDEIntegration
//------------------------------------------------------------
KDEIntegration::KDEIntegration(QObject *parent) : QObject(parent) { }

bool KDEIntegration::ensureWaylandGeometryScript()
{
    if (!KWindowSystem::isPlatformWayland())
        return false;
    if (_waylandGeometryScriptAvailable)
        return true;

#ifdef QTNOTE_DEVEL_KWIN_SCRIPT
    const QString scriptPath = QStringLiteral(QTNOTE_DEVEL_KWIN_SCRIPT);
#else
    const QString scriptPath = QStandardPaths::locate(
        QStandardPaths::GenericDataLocation, QStringLiteral("kwin/scripts/qtnotewindowgeometry/contents/code/main.js"));
#endif
    if (scriptPath.isEmpty()) {
        qCWarning(logKdeIntegration) << "QtNote KWin geometry script was not found";
        return false;
    }

    QDBusInterface scripting(QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting"),
                             QStringLiteral("org.kde.kwin.Scripting"));
    const QString  pluginName = QStringLiteral("qtnotewindowgeometry");
    // Reload even if a previous process loaded the script but failed before run().
    scripting.call(QStringLiteral("unloadScript"), pluginName);

    QDBusReply<int> scriptId = scripting.call(QStringLiteral("loadScript"), scriptPath, pluginName);
    if (!scriptId.isValid() || scriptId.value() < 0) {
        qCWarning(logKdeIntegration) << "Failed to load QtNote KWin geometry script:" << scriptId.error().message();
        return false;
    }
    const QString  scriptObjectPath = QStringLiteral("/Scripting/Script%1").arg(scriptId.value());
    QDBusInterface script(QStringLiteral("org.kde.KWin"), scriptObjectPath, QStringLiteral("org.kde.kwin.Script"));
    const auto     runReply         = script.call(QStringLiteral("run"));
    _waylandGeometryScriptAvailable = runReply.type() != QDBusMessage::ErrorMessage;
    if (!_waylandGeometryScriptAvailable)
        qCWarning(logKdeIntegration) << "Failed to start QtNote KWin geometry script:" << runReply.errorMessage();
    else
        qCInfo(logKdeIntegration) << "QtNote KWin geometry script started:" << scriptObjectPath;
    return _waylandGeometryScriptAvailable;
}

int KDEIntegration::metadataVersion() const { return MetadataVersion; }

PluginMetadata KDEIntegration::metadata()
{
    PluginMetadata md;
    md.id          = pluginId;
    md.icon        = QIcon(":/icons/kde-logo");
    md.name        = "KDE Integration";
    md.description = tr("Provide native look and feel for KDE users");
    md.author      = "Sergei Ilinykh <rion4ik@gmail.com>";
    md.version     = 0x01000000;     // plugin's version 0xXXYYZZPP
    md.minVersion  = 0x020300;       // minimum compatible version of QtNote
    md.maxVersion  = QTNOTE_VERSION; // maximum compatible version of QtNote
    md.homepage    = QUrl("http://ri0n.github.io/QtNote");
    md.extra.insert("de", QStringList() << "kde");
    md.extra.insert("externalTray", true);
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
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), SLOT(activator()));
    timer->setProperty("widget", QVariant::fromValue<QWidget *>(w));
    timer->start(100);
}

static KConfigGroup windowGeometryGroup(const QString &key)
{
    KConfigGroup root(KSharedConfig::openConfig(), QStringLiteral("QtNote Window Geometry"));
    return KConfigGroup(&root, key);
}

WindowGeometryRestoreResult KDEIntegration::restoreWindowGeometry(QWidget *widget, const QString &key)
{
    if (KWindowSystem::isPlatformWayland()) {
        if (!ensureWaylandGeometryScript())
            return WindowGeometryRestoreResult::Unsupported;
        if (!_pendingWindowGeometryKeys.contains(key))
            _pendingWindowGeometryKeys.enqueue(key);
        return WindowGeometryRestoreResult::Pending;
    }
    widget->winId();
    auto *window = widget->windowHandle();
    auto  group  = windowGeometryGroup(key);
    if (!window || !group.exists())
        return WindowGeometryRestoreResult::Unsupported;
    KWindowConfig::restoreWindowSize(window, group);
    KWindowConfig::restoreWindowPosition(window, group);
    widget->resize(window->size());
    return WindowGeometryRestoreResult::Restored;
}

QString KDEIntegration::takePendingWindowGeometryKey()
{
    return _pendingWindowGeometryKeys.isEmpty() ? QString() : _pendingWindowGeometryKeys.dequeue();
}

bool KDEIntegration::saveWindowGeometry(QWidget *widget, const QString &key)
{
    if (KWindowSystem::isPlatformWayland())
        return ensureWaylandGeometryScript();
    auto *window = widget->windowHandle();
    if (!window)
        return false;
    auto group = windowGeometryGroup(key);
    KWindowConfig::saveWindowSize(window, group);
    KWindowConfig::saveWindowPosition(window, group);
    group.sync();
    // KWindowConfig is supplemental on X11. Keep QSettings as the canonical,
    // cross-platform value by asking the caller to perform its normal save too.
    return false;
}

bool KDEIntegration::removeWindowGeometry(const QString &key)
{
    auto group = windowGeometryGroup(key);
    if (group.exists()) {
        group.deleteGroup();
        group.sync();
    }
    return true;
}

void KDEIntegration::activator()
{
    QTimer  *timer = (QTimer *)sender();
    QWidget *w     = sender()->property("widget").value<QWidget *>();

    w->showNormal();
    w->raise();
    w->activateWindow();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (auto *window = w->windowHandle()) {
        if (KWindowSystem::isPlatformWayland())
            KWindowSystem::updateStartupId(window);
        window->requestActivate();
        KWindowSystem::activateWindow(window);
    }
#elif defined(OLD_K_FORCE_ACTIVATE)
    KWindowSystem::forceActiveWindow(w->winId(), 0);
#else
    KX11Extras::forceActiveWindow(w->winId(), 0);
#endif
    timer->deleteLater();
}

bool KDEIntegration::registerGlobalShortcut(const QString &id, const QKeySequence &key, QAction *action)
{
    QAction *act = _shortcuts.value(id);
    if (!act) {
        act = new QAction(action->text(), this);
        act->setObjectName(id);
        _shortcuts.insert(id, act);
    }
    const bool registered = KGlobalAccel::setGlobalShortcut(act, key);
    if (!registered) {
        qCWarning(logKdeIntegration).noquote()
            << "registerGlobalShortcut failed"
            << "id=" << id << "text=" << action->text() << "key=" << key.toString(QKeySequence::NativeText)
            << "platform=" << QGuiApplication::platformName();
    }
    connect(act, SIGNAL(triggered()), action, SLOT(trigger()), Qt::UniqueConnection);
    return registered;
}

bool KDEIntegration::updateGlobalShortcut(const QString &id, const QKeySequence &key)
{
    auto act = _shortcuts.value(id);
    if (!act) {
        qCWarning(logKdeIntegration) << "updateGlobalShortcut: no action for id" << id
                                     << "key=" << key.toString(QKeySequence::NativeText);
        return false;
    }
    const bool registered = KGlobalAccel::setGlobalShortcut(act, key);
    if (!registered) {
        qCWarning(logKdeIntegration).noquote() << "updateGlobalShortcut failed"
                                               << "id=" << id << "key=" << key.toString(QKeySequence::NativeText);
    }
    return registered;
}

void KDEIntegration::setGlobalShortcutEnabled(const QString &id, bool enabled)
{
    auto act = _shortcuts.value(id);
    if (!act) {
        qCWarning(logKdeIntegration) << "setGlobalShortcutEnabled: no action for id" << id << "enabled=" << enabled;
        return;
    }
    act->setEnabled(enabled);
}

} // namespace QtNote
