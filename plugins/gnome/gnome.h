#ifndef GNOMEPLUGIN_H
#define GNOMEPLUGIN_H

#include <QObject>

#include "deintegrationinterface.h"
#include "notificationinterface.h"
#include "qtnoteplugininterface.h"
#include "trayinterface.h"

namespace QtNote {

class GnomeTray;
class PluginHostInterface;

class GnomePlugin : public QObject,
                    public PluginInterface,
                    public TrayInterface,
                    DEIntegrationInterface,
                    public NotificationInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.Gnome")
    Q_INTERFACES(
        QtNote::PluginInterface QtNote::TrayInterface QtNote::DEIntegrationInterface QtNote::NotificationInterface)
public:
    explicit GnomePlugin(QObject *parent = 0);

    int                    metadataVersion() const;
    virtual PluginMetadata metadata();
    void                   setHost(PluginHostInterface *host);

    TrayImpl *initTray(Main *qtnote);
    void      notifyError(const QString &msg);

    void activateWidget(QWidget *w);

private slots:
    void activator();

private:
    GnomeTray           *_tray;
    PluginHostInterface *host;
};

} // namespace QtNote

#endif // GNOMEPLUGIN_H
