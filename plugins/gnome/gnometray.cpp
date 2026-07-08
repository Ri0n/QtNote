#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QMetaType>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>

#include "gnometray.h"
#include "notemanager.h"
#include "qtnote.h"
#include "utils.h"

typedef QPair<QString, QString> NoteIdent;
Q_DECLARE_METATYPE(NoteIdent)

namespace QtNote {

namespace {
    constexpr auto ShellExtensionId = "qtnote@ri0n.github.io";
}

GnomeTray::GnomeTray(Main *qtnote, QObject *parent) : TrayImpl(parent), qtnote(qtnote), contextMenu(0)
{
    sti = new QSystemTrayIcon(QIcon::fromTheme("qtnote", QIcon(":/icons/trayicon")), this);
    connect(sti, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SIGNAL(newNoteTriggered()));

    menuUpdateTimer = new QTimer(this);
    menuUpdateTimer->setInterval(1000);
    menuUpdateTimer->setSingleShot(true);
    connect(menuUpdateTimer, SIGNAL(timeout()), SLOT(rebuildMenu()));

    actQuit    = new QAction(QIcon(":/icons/exit"), tr("&Quit"), this);
    actNew     = new QAction(QIcon(":/icons/new"), tr("&New"), this);
    actAbout   = new QAction(QIcon(":/icons/trayicon"), tr("&About"), this);
    actOptions = new QAction(QIcon(":/icons/options"), tr("&Options"), this);
    actManager = new QAction(QIcon(":/icons/manager"), tr("&Note Manager"), this);

    connect(actQuit, SIGNAL(triggered()), SIGNAL(exitTriggered()));
    connect(actNew, SIGNAL(triggered()), SIGNAL(newNoteTriggered()));
    connect(actManager, SIGNAL(triggered()), SIGNAL(noteManagerTriggered()));
    connect(actOptions, SIGNAL(triggered()), SIGNAL(optionsTriggered()));
    connect(actAbout, SIGNAL(triggered()), SIGNAL(aboutTriggered()));

    advancedMenu = new QMenu;
    advancedMenu->addAction(actOptions);
    advancedMenu->addAction(actManager);
    advancedMenu->addAction(actAbout);
    advancedMenu->addSeparator();
    advancedMenu->addAction(actQuit);
    advancedMenu->setTitle(tr("More.."));

    connect(NoteManager::instance(), SIGNAL(storageAdded(NoteStorage::Ptr)), menuUpdateTimer, SLOT(start()));
    connect(NoteManager::instance(), SIGNAL(storageRemoved(NoteStorage::Ptr)), menuUpdateTimer, SLOT(start()));
    connect(NoteManager::instance(), SIGNAL(storageChanged(NoteStorage::Ptr)), menuUpdateTimer, SLOT(start()));

    contextMenu = new QMenu(tr("Notes"));
    connect(sti, &QObject::destroyed, this, [this]() {
        contextMenu->deleteLater();
        advancedMenu->deleteLater();
    });

    sti->setContextMenu(contextMenu);

    menuUpdateTimer->start();
    sti->show();

    QTimer::singleShot(0, this, &GnomeTray::askEnableShellExtension);
}

GnomeTray::~GnomeTray() { }

void GnomeTray::askEnableShellExtension()
{
    if (!isShellExtensionInstalled() || isShellExtensionEnabled())
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
    if (result == QMessageBox::Yes && !enableShellExtension()) {
        QMessageBox::warning(nullptr, tr("Enable QtNote GNOME Extension"),
                             tr("Failed to enable the QtNote GNOME Shell extension. "
                                "You may need to log out and log back in before enabling it."));
    }
}

bool GnomeTray::isShellExtensionInstalled() const
{
    return !QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                      QLatin1String("gnome-shell/extensions/") + QLatin1String(ShellExtensionId)
                                          + QLatin1String("/metadata.json"),
                                      QStandardPaths::LocateFile)
                .isEmpty();
}

bool GnomeTray::isShellExtensionEnabled() const
{
    const QString executable = QStandardPaths::findExecutable(QLatin1String("gnome-extensions"));
    if (executable.isEmpty())
        return false;

    QProcess process;
    process.start(executable, { QLatin1String("info"), QLatin1String(ShellExtensionId) });
    if (!process.waitForFinished(3000))
        return false;

    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput()).toLower();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0
        && output.contains(QLatin1String("state: enabled"));
}

bool GnomeTray::enableShellExtension() const
{
    const QString executable = QStandardPaths::findExecutable(QLatin1String("gnome-extensions"));
    if (executable.isEmpty())
        return false;

    QProcess process;
    process.start(executable, { QLatin1String("enable"), QLatin1String(ShellExtensionId) });
    if (!process.waitForFinished(5000))
        return false;

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

void GnomeTray::rebuildMenu()
{
    uint                h = 0;
    QSettings           s;
    QList<NoteListItem> notes = NoteManager::instance()->noteList(s.value("ui.menu-notes-amount", 15).toInt());
    for (int i = 0; i < notes.count(); i++) {
        h ^= qHash(notes[i].title);
    }

    if (menuUpdateHash && h == *menuUpdateHash) {
        return; // menu is not changed;
    }
    menuUpdateHash = h;

    contextMenu->clear();
    contextMenu->addAction(actNew);
    contextMenu->addSeparator();
    for (int i = 0; i < notes.count(); i++) {
        QAction *act = contextMenu->addAction(NoteManager::instance()->storage(notes[i].storageId)->noteIcon(),
                                              Utils::cuttedDots(notes[i].title, 48).replace('&', "&&"));
        QVariant v;
        v.setValue<NoteIdent>(NoteIdent(notes[i].storageId, notes[i].id));
        act->setData(v);
        connect(act, SIGNAL(triggered()), SLOT(noteSelected()));
    }
    if (notes.count()) {
        contextMenu->addSeparator();
    }
    contextMenu->addMenu(advancedMenu);
}

void GnomeTray::noteSelected()
{
    NoteIdent ni = ((QAction *)sender())->data().value<NoteIdent>();
    emit      showNoteTriggered(ni.first, ni.second);
}

} // namespace QtNote
