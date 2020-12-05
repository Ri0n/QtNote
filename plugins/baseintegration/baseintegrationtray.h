#ifndef BASEINTEGRATIONTRAY_H
#define BASEINTEGRATIONTRAY_H

#include <QSystemTrayIcon>

#include "qtnote.h"
#include "trayimpl.h"

class QMenu;
class QAction;

namespace QtNote {

class PluginHostInterface;

class BaseIntegrationTray : public TrayImpl {
    Q_OBJECT

    friend class BaseIntegration;

    Main *               qtnote;
    PluginHostInterface *host;
    QSystemTrayIcon *    tray;
    QMenu *              contextMenu;
    QAction *            actQuit, *actNew, *actAbout, *actOptions, *actManager;

public:
    explicit BaseIntegrationTray(Main *qtnote, PluginHostInterface *host, QObject *parent = 0);
    void notifyError(const QString &message);

signals:

public slots:

private slots:
    void showNoteList(QSystemTrayIcon::ActivationReason reason);
};

} // namespace QtNote

#endif // BASEINTEGRATIONTRAY_H
