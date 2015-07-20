#ifndef KDEINTEGRATION_H
#define KDEINTEGRATION_H

#include <QObject>

#include "deintegrationinterface.h"
#include "qtnoteplugininterface.h"
#include "trayinterface.h"
#include "globalshortcutsinterface.h"
#include "notificationinterface.h"

class KAction;

namespace QtNote {

class KDEIntegration : public QObject, public PluginInterface, public NotificationInterface,
        public TrayInterface, public DEIntegrationInterface, public GlobalShortcutsInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.KdeTrayIcon")
#endif
    Q_INTERFACES(QtNote::PluginInterface QtNote::TrayInterface QtNote::DEIntegrationInterface QtNote::GlobalShortcutsInterface QtNote::NotificationInterface)
public:
	explicit KDEIntegration(QObject *parent = 0);

    int metadataVersion() const;
	virtual PluginMetadata metadata();
	TrayImpl* initTray(Main *qtnote);
    void notifyError(const QString &msg);
    void activateWidget(QWidget *w);

	bool registerGlobalShortcut(const QString &id, const QKeySequence &key, QObject *receiver, const char *slot);
	
signals:
	
public slots:

private:
#ifdef USE_KDE5
    QHash<QString, QAction*> _shortcuts;
#else
    QHash<QString, KAction*> _shortcuts;
#endif
};

} // namespace QtNote

#endif // KDEINTEGRATION_H
