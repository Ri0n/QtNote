#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QMenu>
#include <QMetaType>
#include <QSettings>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>

#include "notemanager.h"
#include "qtnote.h"
#include "ubuntutray.h"
#include "utils.h"

typedef QPair<QString, QString> NoteIdent;
Q_DECLARE_METATYPE(NoteIdent)

namespace QtNote {

UbuntuTray::UbuntuTray(Main *qtnote, QObject *parent) : TrayImpl(parent), qtnote(qtnote), contextMenu(0)
{
    menuUpdateTimer = new QTimer(this);
    menuUpdateTimer->setInterval(1000);
    menuUpdateTimer->setSingleShot(true);
    connect(menuUpdateTimer, SIGNAL(timeout()), SLOT(rebuildMenu()));

    sti = new QSystemTrayIcon(QIcon::fromTheme("qtnote", QIcon(":/icons/trayicon")), this);
    connect(sti, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SIGNAL(newNoteTriggered()));

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

    connect(NoteManager::instance(), SIGNAL(storageAdded(StorageItem)), menuUpdateTimer, SLOT(start()));
    connect(NoteManager::instance(), SIGNAL(storageRemoved(StorageItem)), menuUpdateTimer, SLOT(start()));
    connect(NoteManager::instance(), SIGNAL(storageChanged(StorageItem)), menuUpdateTimer, SLOT(start()));
    menuUpdateTimer->start();
}

UbuntuTray::~UbuntuTray()
{
    delete contextMenu;
    delete advancedMenu;
}

void UbuntuTray::rebuildMenu()
{
    uint                h = 0;
    QSettings           s;
    QList<NoteListItem> notes = NoteManager::instance()->noteList(s.value("ui.menu-notes-amount", 15).toInt());
    for (int i = 0; i < notes.count(); i++) {
        h ^= qHash(notes[i].title);
    }

    if (h == menuUpdateHash) {
        return; // menu is not changed;
    }

    delete sti->contextMenu();
    QMenu *contextMenu = new QMenu(tr("Notes"));
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

    sti->setContextMenu(contextMenu);
    sti->show();
}

void UbuntuTray::noteSelected()
{
    NoteIdent ni = ((QAction *)sender())->data().value<NoteIdent>();
    emit      showNoteTriggered(ni.first, ni.second);
}

} // namespace QtNote
