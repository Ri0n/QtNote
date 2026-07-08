#ifndef GNOMEPLUGIN_H
#define GNOMEPLUGIN_H

#include <QObject>

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

    int                    metadataVersion() const;
    virtual PluginMetadata metadata();
    void                   setHost(PluginHostInterface *host);

    void notifyError(const QString &msg);

    void activateWidget(QWidget *w);

private slots:
    void askEnableShellExtension();
    void activator();

private:
    bool isShellExtensionInstalled() const;
    bool isShellExtensionEnabled() const;
    bool enableShellExtension() const;

    PluginHostInterface *host;
};

} // namespace QtNote

#endif // GNOMEPLUGIN_H
