#ifndef KDEINTEGRATION_H
#define KDEINTEGRATION_H

#include <QObject>

#include "deintegrationinterface.h"
#include "globalshortcutsinterface.h"
#include "notificationinterface.h"
#include "qtnoteplugininterface.h"
#include "trayinterface.h"

class KAction;

namespace QtNote {

class PluginHostInterface;

class KDEIntegration : public QObject,
                       public PluginInterface,
                       public NotificationInterface,
                       public TrayInterface,
                       public DEIntegrationInterface,
                       public GlobalShortcutsInterface {
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.KdeTrayIcon")
#endif
    Q_INTERFACES(QtNote::PluginInterface QtNote::TrayInterface QtNote::DEIntegrationInterface
                     QtNote::GlobalShortcutsInterface QtNote::NotificationInterface)
public:
    explicit KDEIntegration(QObject *parent = 0);

    int                    metadataVersion() const override;
    virtual PluginMetadata metadata() override;
    void                   setHost(PluginHostInterface *host) override;

    TrayImpl *initTray(Main *qtnote) override;
    void      notifyError(const QString &msg) override;
    void      activateWidget(QWidget *w) override;

    bool registerGlobalShortcut(const QString &id, const QKeySequence &key, QAction *action) override;
    bool updateGlobalShortcut(const QString &id, const QKeySequence &key) override;
    void setGlobalShortcutEnabled(const QString &id, bool enabled = true) override;

signals:

public slots:

private:
    QHash<QString, QAction *> _shortcuts;
};

} // namespace QtNote

#endif // KDEINTEGRATION_H
