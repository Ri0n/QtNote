#ifndef UBUNTUTRAY_H
#define UBUNTUTRAY_H

#include "trayimpl.h"

#include <optional>

class QAction;
class QSystemTrayIcon;
class QTimer;
class QMenu;

namespace QtNote {

class Main;

class UbuntuTray : public TrayImpl {
    Q_OBJECT
public:
    explicit UbuntuTray(Main *qtnote, QObject *parent);
    ~UbuntuTray();

signals:

public slots:

private slots:
    void rebuildMenu();
    void noteSelected();

private:
    friend class UbuntuPlugin;
    Main               *qtnote;
    QSystemTrayIcon    *sti = nullptr;
    QAction            *actQuit, *actNew, *actAbout, *actOptions, *actManager;
    QTimer             *menuUpdateTimer;
    QMenu              *contextMenu;
    QMenu              *advancedMenu;
    std::optional<uint> menuUpdateHash;
};

}

#endif // UBUNTUTRAY_H
