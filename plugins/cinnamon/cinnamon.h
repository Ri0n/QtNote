#ifndef CINNAMONPLUGIN_H
#define CINNAMONPLUGIN_H

#include <QObject>
#include <QQueue>

#include "deintegrationinterface.h"
#include "notificationinterface.h"
#include "qtnoteplugininterface.h"

namespace QtNote {

class PluginHostInterface;

class CinnamonPlugin : public QObject, public PluginInterface, DEIntegrationInterface, public NotificationInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.Cinnamon")
    Q_INTERFACES(QtNote::PluginInterface QtNote::DEIntegrationInterface QtNote::NotificationInterface)
public:
    explicit CinnamonPlugin(QObject *parent = nullptr);

    int            metadataVersion() const;
    PluginMetadata metadata() override;
    void           setHost(PluginHostInterface *host);

    void                        notifyError(const QString &message) override;
    void                        activateWidget(QWidget *w) override;
    WindowGeometryRestoreResult restoreWindowGeometry(QWidget *w, const QString &key) override;
    bool                        saveWindowGeometry(QWidget *w, const QString &key) override;
    bool                        removeWindowGeometry(const QString &key) override;
    QString                     takePendingWindowGeometryKey() override;
    void                        windowGeometryBridgeReady() override;

private slots:
    void ensureAppletEnabled();
    void activator();

private:
    bool isAppletInstalled() const;
    bool isAppletEnabled() const;
    bool enableApplet() const;

    PluginHostInterface *host = nullptr;
    QQueue<QString>      pendingWindowGeometryKeys;
    bool                 geometryBridgeReady = false;
};

} // namespace QtNote

#endif // CINNAMONPLUGIN_H
