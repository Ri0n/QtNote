#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <utility>
#else
#include <KStatusNotifierItem>
#include <QApplication>
#include <QMenu>
#include <QScreen>
#include <QStyle>
#endif
#include <QSettings>

#include "kdeintegrationtray.h"
#include "notemanager.h"
#include "qtnote.h"
#include "trayiconutils.h"
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include "utils.h"
#endif

namespace QtNote {

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
namespace {
    constexpr auto PlasmoidId = "com.github.ri0n.qtnote";
#ifdef QTNOTE_DEVEL
    constexpr auto DevelopmentPlasmoidPackageSetting = "developmentPlasmoidPackageLink";
    constexpr auto DevelopmentPlasmoidQmlSetting     = "developmentPlasmoidQmlLinks";
    constexpr auto DevelopmentPlasmoidOldQmlSetting  = "developmentPlasmoidQmlLink";
    constexpr auto DevelopmentPlasmoidMarker         = ".qtnote-devel-package";
#endif
}

KDEIntegrationTray::KDEIntegrationTray(Main *qtnote, QObject *parent) : TrayImpl(parent)
{
    Q_UNUSED(qtnote)

#ifdef QTNOTE_DEVEL
    auto *plasmaShellWatcher
        = new QDBusServiceWatcher(QLatin1String("org.kde.plasmashell"), QDBusConnection::sessionBus(),
                                  QDBusServiceWatcher::WatchForRegistration, this);
    connect(plasmaShellWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this] {
        QTimer::singleShot(1500, this, &KDEIntegrationTray::resetDevelopmentPlasmoidOnPanel);
        QTimer::singleShot(3500, this, &KDEIntegrationTray::resetDevelopmentPlasmoidOnPanel);
    });
#endif

    QTimer::singleShot(0, this, &KDEIntegrationTray::askAddPlasmoid);
}

KDEIntegrationTray::~KDEIntegrationTray()
{
#ifdef QTNOTE_DEVEL
    removeDevelopmentPlasmoidFromPanel();
    cleanupDevelopmentPlasmoidLinks();
#endif
}

void KDEIntegrationTray::askAddPlasmoid()
{
#ifdef QTNOTE_DEVEL
    if (!ensureDevelopmentPlasmoidLinks())
        return;

    resetDevelopmentPlasmoidOnPanel();
    return;
#else
    if (!isPlasmoidInstalled())
        return;

    QSettings settings;
    settings.beginGroup(QLatin1String("kdeintegration"));
    if (settings.value(QLatin1String("plasmoidAddAsked"), false).toBool())
        return;
    settings.setValue(QLatin1String("plasmoidAddAsked"), true);

    auto result = QMessageBox::question(nullptr, tr("Add QtNote Widget"),
                                        tr("QtNote can add a native Plasma widget to the panel. "
                                           "It provides Wayland-friendly access to recent notes."),
                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (result == QMessageBox::Yes)
        addPlasmoidToPanel();
#endif
}

void KDEIntegrationTray::addPlasmoidToPanel()
{
    static const auto script = QStringLiteral(R"(
var pluginId = "%1";

function isPlugin(widget) {
    return widget.type === pluginId || widget.pluginName === pluginId;
}

function findInContainment(containment) {
    if (!containment) {
        return false;
    }

    var widgets = containment.widgets();
    for (var i = 0; i < widgets.length; ++i) {
        var widget = widgets[i];
        if (isPlugin(widget)) {
            return true;
        }
    }

    return false;
}

var panelList = panels();
var found = false;
for (var i = 0; i < panelList.length; ++i) {
    found = findInContainment(panelList[i]) || found;
}

if (!found) {
    if (panelList.length > 0) {
        panelList[0].addWidget(pluginId);
    }
}
)");

    QDBusInterface plasmaShell(QLatin1String("org.kde.plasmashell"), QLatin1String("/PlasmaShell"),
                               QLatin1String("org.kde.PlasmaShell"), QDBusConnection::sessionBus());

    if (!plasmaShell.isValid()) {
        qWarning("Failed to access plasmashell D-Bus scripting interface");
        return;
    }

    plasmaShell.asyncCall(QLatin1String("evaluateScript"), script.arg(QLatin1String(PlasmoidId)));
}

bool KDEIntegrationTray::isPlasmoidInstalled() const
{
    return !QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                      QLatin1String("plasma/plasmoids/") + QLatin1String(PlasmoidId)
                                          + QLatin1String("/metadata.json"),
                                      QStandardPaths::LocateFile)
                .isEmpty();
}

#ifdef QTNOTE_DEVEL
bool KDEIntegrationTray::ensureDevelopmentPlasmoidLinks()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (dataDir.isEmpty())
        return false;

    QDir localPrefix(dataDir);
    if (!localPrefix.cdUp())
        return false;

    const QString     packageLink = dataDir + QLatin1String("/plasma/plasmoids/") + QLatin1String(PlasmoidId);
    const QString     qmlSource   = QLatin1String(QTNOTE_DEVEL_PLASMOID_QML_DIR);
    const QStringList qmlLinks    = {
        localPrefix.absoluteFilePath(QLatin1String(QTNOTE_DEVEL_PLASMOID_QML_INSTALL_DIR)),
        dataDir + QLatin1String("/QtProject/qml/com/github/ri0n/qtnote"),
    };

    cleanupDevelopmentPlasmoidLinks();

    QSettings settings;
    settings.beginGroup(QLatin1String("kdeintegration"));
    settings.setValue(QLatin1String(DevelopmentPlasmoidPackageSetting), packageLink);
    settings.setValue(QLatin1String(DevelopmentPlasmoidQmlSetting), qmlLinks);

    const bool packageOk
        = updateDevelopmentPackage(QLatin1String(QTNOTE_DEVEL_PLASMOID_PACKAGE_DIR), packageLink, qmlSource);
    bool qmlOk = true;
    for (const auto &qmlLink : qmlLinks)
        qmlOk = updateDevelopmentLink(qmlSource, qmlLink) && qmlOk;

    if (!packageOk || !qmlOk)
        cleanupDevelopmentPlasmoidLinks();

    if (packageOk) {
        clearDevelopmentPlasmaCache();

        const QString kbuildsycoca = QStandardPaths::findExecutable(QLatin1String("kbuildsycoca6"));
        if (!kbuildsycoca.isEmpty())
            QProcess::startDetached(kbuildsycoca, { QLatin1String("--noincremental") });
    }

    return packageOk && qmlOk;
}

void KDEIntegrationTray::cleanupDevelopmentPlasmoidLinks()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("kdeintegration"));

    const QString packageLink = settings.value(QLatin1String(DevelopmentPlasmoidPackageSetting)).toString();
    QStringList   qmlLinks    = settings.value(QLatin1String(DevelopmentPlasmoidQmlSetting)).toStringList();
    const QString oldQmlLink  = settings.value(QLatin1String(DevelopmentPlasmoidOldQmlSetting)).toString();
    if (!oldQmlLink.isEmpty())
        qmlLinks.append(oldQmlLink);

    const auto removeExpectedLink = [](const QString &linkPath, const QString &targetPath) {
        if (linkPath.isEmpty())
            return;

        QFileInfo linkInfo(linkPath);
        if (linkInfo.isSymLink()
            && QFileInfo(linkInfo.symLinkTarget()).canonicalFilePath() == QFileInfo(targetPath).canonicalFilePath()) {
            QFile::remove(linkPath);
        }
    };

    QFileInfo packageInfo(packageLink);
    if (packageInfo.isSymLink()) {
        removeExpectedLink(packageLink, QLatin1String(QTNOTE_DEVEL_PLASMOID_PACKAGE_DIR));
    } else if (QFileInfo::exists(packageLink + QLatin1Char('/') + QLatin1String(DevelopmentPlasmoidMarker))) {
        QDir(packageLink).removeRecursively();
    }

    for (const auto &qmlLink : std::as_const(qmlLinks))
        removeExpectedLink(qmlLink, QLatin1String(QTNOTE_DEVEL_PLASMOID_QML_DIR));

    settings.remove(QLatin1String(DevelopmentPlasmoidPackageSetting));
    settings.remove(QLatin1String(DevelopmentPlasmoidQmlSetting));
    settings.remove(QLatin1String(DevelopmentPlasmoidOldQmlSetting));
}

void KDEIntegrationTray::clearDevelopmentPlasmaCache()
{
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    if (cacheDir.isEmpty())
        return;

    QDir(cacheDir + QLatin1String("/plasmashell/qmlcache")).removeRecursively();
}

void KDEIntegrationTray::resetDevelopmentPlasmoidOnPanel()
{
    static const auto script = QStringLiteral(R"(
var pluginId = "%1";

function isPlugin(widget) {
    return widget.type === pluginId || widget.pluginName === pluginId;
}

function removeDirectFromContainment(containment) {
    var widgets = containment.widgets();
    for (var i = widgets.length - 1; i >= 0; --i) {
        var widget = widgets[i];
        if (isPlugin(widget)) {
            widget.remove();
        }
    }
}

var containmentList = desktops().concat(panels());
for (var k = 0; k < containmentList.length; ++k) {
    removeDirectFromContainment(containmentList[k]);
}
)");

    QDBusInterface plasmaShell(QLatin1String("org.kde.plasmashell"), QLatin1String("/PlasmaShell"),
                               QLatin1String("org.kde.PlasmaShell"), QDBusConnection::sessionBus());

    if (!plasmaShell.isValid()) {
        qWarning("Failed to access plasmashell D-Bus scripting interface");
        return;
    }

    plasmaShell.asyncCall(QLatin1String("evaluateScript"), script.arg(QLatin1String(PlasmoidId)));
    QTimer::singleShot(1000, this, &KDEIntegrationTray::addPlasmoidToPanel);
    QTimer::singleShot(2500, this, &KDEIntegrationTray::addPlasmoidToPanel);
    QTimer::singleShot(5000, this, &KDEIntegrationTray::addPlasmoidToPanel);
}

void KDEIntegrationTray::removeDevelopmentPlasmoidFromPanel()
{
    static const auto script = QStringLiteral(R"(
var pluginId = "%1";

function isPlugin(widget) {
    return widget.type === pluginId || widget.pluginName === pluginId;
}

var containmentList = desktops().concat(panels());
for (var i = 0; i < containmentList.length; ++i) {
    var widgetList = containmentList[i].widgets();
    for (var j = widgetList.length - 1; j >= 0; --j) {
        var widget = widgetList[j];
        if (isPlugin(widget)) {
            widget.remove();
        }
    }
}
)");

    QDBusInterface plasmaShell(QLatin1String("org.kde.plasmashell"), QLatin1String("/PlasmaShell"),
                               QLatin1String("org.kde.PlasmaShell"), QDBusConnection::sessionBus());

    if (!plasmaShell.isValid()) {
        qWarning("Failed to access plasmashell D-Bus scripting interface");
        return;
    }

    plasmaShell.asyncCall(QLatin1String("evaluateScript"), script.arg(QLatin1String(PlasmoidId)));
}

bool KDEIntegrationTray::updateDevelopmentLink(const QString &sourcePath, const QString &linkPath)
{
    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists())
        return false;

    QFileInfo linkInfo(linkPath);
    if (linkInfo.exists() || linkInfo.isSymLink()) {
        if (!linkInfo.isSymLink()) {
            qWarning("Refusing to replace non-symlink development plasmoid path: %s", qPrintable(linkPath));
            return false;
        }
        if (!QFile::remove(linkPath))
            return false;
    }

    QDir parentDir = QFileInfo(linkPath).dir();
    if (!parentDir.mkpath(QLatin1String(".")))
        return false;

    return QFile::link(sourceInfo.absoluteFilePath(), linkPath);
}

bool KDEIntegrationTray::updateDevelopmentFile(const QString &sourcePath, const QString &filePath)
{
    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.isFile())
        return false;

    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() || fileInfo.isSymLink()) {
        if (fileInfo.isDir() && !fileInfo.isSymLink()) {
            qWarning("Refusing to replace directory development plasmoid path: %s", qPrintable(filePath));
            return false;
        }
        if (!QFile::remove(filePath))
            return false;
    }

    QDir parentDir = QFileInfo(filePath).dir();
    if (!parentDir.mkpath(QLatin1String(".")))
        return false;

    return QFile::copy(sourceInfo.absoluteFilePath(), filePath);
}

bool KDEIntegrationTray::updateDevelopmentMainQml(const QString &sourcePath, const QString &filePath)
{
    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly))
        return false;

    QByteArray content = source.readAll();
    source.close();

    content.replace("import plasma.applet.com.github.ri0n.qtnote 1.0 as QtNote", "import \"QtNote\" as QtNote");

    const QString iconPath
        = QFileInfo(QFileInfo(sourcePath)
                        .dir()
                        .absoluteFilePath(QLatin1String("../../../../../../libqtnote/images/qtnote-symbolic.svg")))
              .canonicalFilePath();
    content.replace("Plasmoid.icon: \"qtnote-symbolic\"",
                    "Plasmoid.icon: \"" + QUrl::fromLocalFile(iconPath).toEncoded() + "\"");

    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() || fileInfo.isSymLink()) {
        if (fileInfo.isDir() && !fileInfo.isSymLink()) {
            qWarning("Refusing to replace directory development plasmoid path: %s", qPrintable(filePath));
            return false;
        }
        if (!QFile::remove(filePath))
            return false;
    }

    QDir parentDir = QFileInfo(filePath).dir();
    if (!parentDir.mkpath(QLatin1String(".")))
        return false;

    return updateDevelopmentFileContent(filePath, content);
}

bool KDEIntegrationTray::updateDevelopmentFileContent(const QString &filePath, const QByteArray &content)
{
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() || fileInfo.isSymLink()) {
        if (fileInfo.isDir() && !fileInfo.isSymLink()) {
            qWarning("Refusing to replace directory development plasmoid path: %s", qPrintable(filePath));
            return false;
        }
        if (!QFile::remove(filePath))
            return false;
    }

    QDir parentDir = QFileInfo(filePath).dir();
    if (!parentDir.mkpath(QLatin1String(".")))
        return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    return file.write(content) == content.size();
}

bool KDEIntegrationTray::updateDevelopmentQmlBackend(const QString &sourcePath, const QString &backendPath)
{
    QDir sourceDir(sourcePath);
    if (!sourceDir.exists())
        return false;

    QDir backendDir(backendPath);
    if (!backendDir.mkpath(QLatin1String(".")))
        return false;

    return updateDevelopmentFile(sourceDir.absoluteFilePath(QLatin1String("qmldir")),
                                 backendDir.absoluteFilePath(QLatin1String("qmldir")))
        && updateDevelopmentLink(sourceDir.absoluteFilePath(QLatin1String("libplasmoid_qtnote_devel.so")),
                                 backendDir.absoluteFilePath(QLatin1String("libplasmoid_qtnote_devel.so")))
        && updateDevelopmentLink(sourceDir.absoluteFilePath(QLatin1String("plasmoid_qtnote_devel.qmltypes")),
                                 backendDir.absoluteFilePath(QLatin1String("plasmoid_qtnote_devel.qmltypes")));
}

bool KDEIntegrationTray::updateDevelopmentPackage(const QString &sourcePath, const QString &packagePath,
                                                  const QString &qmlSourcePath)
{
    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.isDir())
        return false;

    QFileInfo packageInfo(packagePath);
    if (packageInfo.exists() || packageInfo.isSymLink()) {
        if (packageInfo.isSymLink()) {
            if (!QFile::remove(packagePath))
                return false;
        } else {
            const QString markerPath   = packagePath + QLatin1Char('/') + QLatin1String(DevelopmentPlasmoidMarker);
            const QString metadataPath = packagePath + QLatin1String("/metadata.json");
            const QString sourceMetadataPath = sourceInfo.absoluteFilePath() + QLatin1String("/metadata.json");
            const QString mainQmlPath        = packagePath + QLatin1String("/contents/ui/main.qml");
            const QString sourceMainQmlPath  = sourceInfo.absoluteFilePath() + QLatin1String("/contents/ui/main.qml");

            const QFileInfo metadataInfo(metadataPath);
            const QFileInfo mainQmlInfo(mainQmlPath);
            const bool      isPreviousDevelopmentPackage = QFileInfo::exists(markerPath)
                || (metadataInfo.isSymLink()
                    && QFileInfo(metadataInfo.symLinkTarget()).canonicalFilePath()
                        == QFileInfo(sourceMetadataPath).canonicalFilePath())
                || (mainQmlInfo.isSymLink()
                    && QFileInfo(mainQmlInfo.symLinkTarget()).canonicalFilePath()
                        == QFileInfo(sourceMainQmlPath).canonicalFilePath());

            if (!isPreviousDevelopmentPackage) {
                qWarning("Refusing to replace non-development plasmoid package path: %s", qPrintable(packagePath));
                return false;
            }

            if (!QDir(packagePath).removeRecursively())
                return false;
        }
    }

    QDir packageDir(packagePath);
    if (!packageDir.mkpath(QLatin1String("contents/ui/QtNote"))) {
        return false;
    }

    QFile marker(packageDir.absoluteFilePath(QLatin1String(DevelopmentPlasmoidMarker)));
    if (!marker.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    marker.write("QtNote development package\n");
    marker.close();

    if (!updateDevelopmentFile(sourceInfo.absoluteFilePath() + QLatin1String("/metadata.json"),
                               packageDir.absoluteFilePath(QLatin1String("metadata.json"))))
        return false;

    QDir       sourceUi(sourceInfo.absoluteFilePath() + QLatin1String("/contents/ui"));
    QDir       targetUi(packageDir.absoluteFilePath(QLatin1String("contents/ui")));
    const auto uiFiles = sourceUi.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const auto &uiFile : uiFiles) {
        const QString targetPath = targetUi.absoluteFilePath(uiFile.fileName());
        const bool    copied     = uiFile.fileName() == QLatin1String("main.qml")
                   ? updateDevelopmentMainQml(uiFile.absoluteFilePath(), targetPath)
                   : updateDevelopmentFile(uiFile.absoluteFilePath(), targetPath);
        if (!copied)
            return false;
    }

    return updateDevelopmentQmlBackend(qmlSourcePath, targetUi.absoluteFilePath(QLatin1String("QtNote")));
}
#endif

#else

KDEIntegrationTray::KDEIntegrationTray(Main *qtnote, QObject *parent) : TrayImpl(parent), qtnote(qtnote)
{
    sni = new KStatusNotifierItem("qtnote", this);
    sni->setIconByName(TrayIconUtils::themedTrayIconName());
    sni->setStatus(KStatusNotifierItem::Active);
    sni->setTitle("Notes");

    auto *contextMenu = new QMenu;
    sni->setContextMenu(contextMenu);

    actNew = new QAction(QIcon(":/icons/new"), tr("&New"), this);
    contextMenu->addAction(actNew);
    contextMenu->addSeparator();
    contextMenu->addAction(QIcon(":/icons/manager"), tr("&Note Manager"), this, SIGNAL(noteManagerTriggered()));
    contextMenu->addAction(QIcon(":/icons/options"), tr("&Options"), this, SIGNAL(optionsTriggered()));
    contextMenu->addAction(QIcon(":/icons/trayicon"), tr("&About"), this, SIGNAL(aboutTriggered()));

    connect(actNew, SIGNAL(triggered()), SIGNAL(newNoteTriggered()));
    connect(sni, SIGNAL(activateRequested(bool, QPoint)), SLOT(showNotes(bool, QPoint)));
    connect(sni, SIGNAL(secondaryActivateRequested(QPoint)), SIGNAL(newNoteTriggered()));
}

KDEIntegrationTray::~KDEIntegrationTray() = default;

void KDEIntegrationTray::showNotes(bool active, const QPoint &pos)
{
    Q_UNUSED(active)
    if (currentMenu) {
        currentMenu->close();
        return;
    }

    QMenu menu;
    menu.addAction(actNew);
    menu.addSeparator();
    QSettings settings;
    auto      notes = NoteManager::instance()->noteList(settings.value("ui.menu-notes-amount", 15).toInt());
    for (int i = 0; i < notes.count(); ++i) {
        menu.addAction(NoteManager::instance()->storage(notes[i].storageId)->noteIcon(),
                       Utils::cuttedDots(notes[i].title, 48).replace('&', "&&"))
            ->setData(i);
    }

    menu.show();
    qtnote->activateWidget(&menu);
    QRect desktopRect = QGuiApplication::screenAt(QCursor::pos())->geometry();
    QRect menuRect    = menu.geometry();
    menuRect.setSize(menu.sizeHint());
    const QPoint menuPosition = pos - QPoint(menuRect.width() / 2, 0);
    if (pos.y() < desktopRect.height() / 2)
        menuRect.moveTopLeft(menuPosition);
    else
        menuRect.moveBottomLeft(menuPosition);

    if (menuRect.right() > desktopRect.right())
        menuRect.moveRight(desktopRect.right());
    if (menuRect.bottom() > desktopRect.bottom())
        menuRect.moveBottom(desktopRect.bottom());
    if (menuRect.left() < desktopRect.left())
        menuRect.moveLeft(desktopRect.left());
    if (menuRect.top() < desktopRect.top())
        menuRect.moveTop(desktopRect.top());

    currentMenu     = &menu;
    QAction *action = menu.exec(menuRect.topLeft());
    currentMenu     = nullptr;

    if (action && action != actNew) {
        const auto &note = notes[action->data().toInt()];
        emit        showNoteTriggered(note.storageId, note.id);
    }
}

#endif

} // namespace QtNote
