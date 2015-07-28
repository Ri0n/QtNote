#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>

#include "deintegrationinterface.h"
#include "trayinterface.h"
#include "qtnoteplugininterface.h"
#include "globalshortcutsinterface.h"
#include "notificationinterface.h"

namespace QtNote {

class Main;

class BaseIntegration : public QObject, public PluginInterface, public DEIntegrationInterface,
        public TrayInterface, public NotificationInterface, public GlobalShortcutsInterface
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.BaseIntegration")
#endif
    Q_INTERFACES(QtNote::PluginInterface QtNote::DEIntegrationInterface QtNote::TrayInterface QtNote::GlobalShortcutsInterface QtNote::NotificationInterface)
public:
    explicit BaseIntegration(QObject *parent = 0);

    int metadataVersion() const;
    PluginMetadata metadata();

    void activateWidget(QWidget *w);
    TrayImpl* initTray(Main *qtnote);
    void notifyError(const QString &message);
    bool registerGlobalShortcut(const QString &id, const QKeySequence &key, QObject *receiver, const char *slot);
    bool updateGlobalShortcut(const QString &id, const QKeySequence &key);

public slots:

private:
    TrayImpl *tray;

};

} // namespace QtNote

#endif // TRAYICON_H
