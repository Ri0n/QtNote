#ifndef GNOMETRAY_H
#define GNOMETRAY_H

#include "trayimpl.h"

#include <optional>

class QAction;
class QSystemTrayIcon;
class QTimer;
class QMenu;

namespace QtNote {

class Main;

class GnomeTray : public TrayImpl {
    Q_OBJECT
public:
    explicit GnomeTray(Main *qtnote, QObject *parent);
    ~GnomeTray();

signals:

public slots:

private slots:
    void rebuildMenu();
    void noteSelected();

private:
    friend class GnomePlugin;
    Main               *qtnote;
    QSystemTrayIcon    *sti = nullptr;
    QAction            *actQuit, *actNew, *actAbout, *actOptions, *actManager;
    QTimer             *menuUpdateTimer;
    QMenu              *contextMenu;
    QMenu              *advancedMenu;
    std::optional<uint> menuUpdateHash;
};

}

#endif // GNOMETRAY_H
