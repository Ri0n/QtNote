#ifndef KDEINTEGRATIONTRAY_H
#define KDEINTEGRATIONTRAY_H

#include "trayimpl.h"

class QAction;
class QMenu;
class KStatusNotifierItem;

namespace QtNote {

class Main;

class KDEIntegrationTray : public TrayImpl {
    Q_OBJECT
public:
    explicit KDEIntegrationTray(Main *qtnote, QObject *parent);

signals:

public slots:

private slots:
    void showNotes(bool active, const QPoint &pos);

private:
    Main                *qtnote;
    KStatusNotifierItem *sni;
    QAction             *actNew;
    QMenu               *currentMenu = nullptr;
};

}

#endif // KDEINTEGRATIONTRAY_H
