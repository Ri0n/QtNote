#ifndef GNOMEPLUGIN_H
#define GNOMEPLUGIN_H

#include <QHash>
#include <QKeySequence>
#include <QObject>

#include "deintegrationinterface.h"
#include "globalshortcutsinterface.h"
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
                    public NotificationInterface,
                    public GlobalShortcutsInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.Gnome")
    Q_INTERFACES(QtNote::PluginInterface QtNote::TrayInterface QtNote::DEIntegrationInterface
                     QtNote::NotificationInterface QtNote::GlobalShortcutsInterface)
public:
    explicit GnomePlugin(QObject *parent = 0);

    int                    metadataVersion() const;
    virtual PluginMetadata metadata();
    void                   setHost(PluginHostInterface *host);

    TrayImpl *initTray(Main *qtnote);
    void      notifyError(const QString &msg);

    void activateWidget(QWidget *w);

    bool registerGlobalShortcut(const QString &id, const QKeySequence &key, QAction *action) override;
    bool updateGlobalShortcut(const QString &id, const QKeySequence &key) override;
    void setGlobalShortcutEnabled(const QString &id, bool enabled = true) override;

private slots:
    void activator();

private:
    bool setShellShortcut(const QString &id, const QKeySequence &key);

    GnomeTray                   *_tray;
    PluginHostInterface         *host;
    QHash<QString, QKeySequence> _shortcuts;
    QHash<QString, bool>         _shortcutEnabled;
};

} // namespace QtNote

#endif // GNOMEPLUGIN_H
