#ifndef MACOSXPLUGIN_H
#define MACOSXPLUGIN_H

#include <QObject>

#include "deintegrationinterface.h"
#include "notificationinterface.h"
#include "qtnoteplugininterface.h"
#include "trayinterface.h"

namespace QtNote {

class MacOSXTray;
class PluginHostInterface;

class MacOSXPlugin : public QObject,
                     public PluginInterface,
                     public TrayInterface,
                     public NotificationInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.MacOSX")
    Q_INTERFACES(
        QtNote::PluginInterface QtNote::TrayInterface QtNote::NotificationInterface)
public:
    explicit MacOSXPlugin(QObject *parent = 0);

    int                    metadataVersion() const;
    virtual PluginMetadata metadata();
    void                   setHost(PluginHostInterface *host);

    TrayImpl *initTray(Main *qtnote);
    void      notifyError(const QString &msg);

private:
    MacOSXTray *         _tray;
    PluginHostInterface *host;
};

} // namespace QtNote

#endif // MACOSXPLUGIN_H
