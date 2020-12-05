#ifndef UBUNTUPLUGIN_H
#define UBUNTUPLUGIN_H

#include <QObject>

#include "deintegrationinterface.h"
#include "notificationinterface.h"
#include "qtnoteplugininterface.h"
#include "trayinterface.h"

namespace QtNote {

class UbuntuTray;

class UbuntuPlugin : public QObject,
                     public PluginInterface,
                     public TrayInterface,
                     DEIntegrationInterface,
                     public NotificationInterface {
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.Ubuntu")
#endif
    Q_INTERFACES(
        QtNote::PluginInterface QtNote::TrayInterface QtNote::DEIntegrationInterface QtNote::NotificationInterface)
public:
    explicit UbuntuPlugin(QObject *parent = 0);

    int                    metadataVersion() const;
    virtual PluginMetadata metadata();

    TrayImpl *initTray(Main *qtnote);
    void      notifyError(const QString &msg);

    void activateWidget(QWidget *w);

private slots:
    void activator();

private:
    UbuntuTray *_tray;
};

} // namespace QtNote

#endif // UBUNTUPLUGIN_H
