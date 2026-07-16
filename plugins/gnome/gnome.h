#ifndef GNOMEPLUGIN_H
#define GNOMEPLUGIN_H

#include <QObject>
#include <QQueue>

#include "deintegrationinterface.h"
#include "notificationinterface.h"
#include "qtnoteplugininterface.h"

namespace QtNote {

class PluginHostInterface;

class GnomePlugin : public QObject, public PluginInterface, DEIntegrationInterface, public NotificationInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.Gnome")
    Q_INTERFACES(QtNote::PluginInterface QtNote::DEIntegrationInterface QtNote::NotificationInterface)
public:
    explicit GnomePlugin(QObject *parent = 0);

    int                    metadataVersion() const override;
    virtual PluginMetadata metadata() override;
    void                   setHost(PluginHostInterface *host) override;

    void notifyError(const QString &msg) override;

    void                        activateWidget(QWidget *w) override;
    WindowGeometryRestoreResult restoreWindowGeometry(QWidget *w, const QString &key) override;
    bool                        saveWindowGeometry(QWidget *w, const QString &key) override;
    bool                        removeWindowGeometry(const QString &key) override;
    QString                     takePendingWindowGeometryKey() override;
    void                        windowGeometryBridgeReady() override;

private slots:
    void askEnableShellExtension();
    void activator();

private:
    bool isShellExtensionInstalled() const;
    bool isShellExtensionEnabled() const;
    bool enableShellExtension() const;
    bool geometryExtensionAvailable();

    PluginHostInterface *host = nullptr;
    QQueue<QString>      pendingWindowGeometryKeys;
    bool                 shellExtensionEnabled = false;
    bool                 geometryBridgeReady   = false;
};

} // namespace QtNote

#endif // GNOMEPLUGIN_H
