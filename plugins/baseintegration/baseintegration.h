#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>

#include "deintegrationinterface.h"
#include "globalshortcutsinterface.h"
#include "notificationinterface.h"
#include "qtnoteplugininterface.h"
#include "trayinterface.h"

class QxtGlobalShortcut;

namespace QtNote {

class Main;

class BaseIntegration : public QObject,
                        public PluginInterface,
                        public DEIntegrationInterface,
                        public TrayInterface,
                        public NotificationInterface,
                        public GlobalShortcutsInterface {
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.BaseIntegration")
#endif
    Q_INTERFACES(QtNote::PluginInterface QtNote::DEIntegrationInterface QtNote::TrayInterface
                     QtNote::GlobalShortcutsInterface QtNote::NotificationInterface)
public:
    explicit BaseIntegration(QObject *parent = 0);

    int            metadataVersion() const;
    PluginMetadata metadata();
    void           setHost(PluginHostInterface *host);

    void      activateWidget(QWidget *w);
    TrayImpl *initTray(Main *qtnote);
    void      notifyError(const QString &message);

    bool registerGlobalShortcut(const QString &id, const QKeySequence &key, QAction *action);
    bool updateGlobalShortcut(const QString &id, const QKeySequence &key);
    void setGlobalShortcutEnabled(const QString &id, bool enabled = true);

public slots:

private:
    PluginHostInterface *               host;
    TrayImpl *                          tray;
    QHash<QString, QxtGlobalShortcut *> _shortcuts;
};

} // namespace QtNote

#endif // TRAYICON_H
