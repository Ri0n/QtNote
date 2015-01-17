#ifndef KDEINTEGRATION_H
#define KDEINTEGRATION_H

#include <QObject>

#include "deintegrationinterface.h"
#include "qtnoteplugininterface.h"
#include "trayinterface.h"
#include "globalshortcutsinterface.h"

class KAction;

namespace QtNote {

class KDEIntegration : public QObject, public QtNotePluginInterface, public TrayInterface,
		public DEIntegrationInterface, public GlobalShortcutsInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.KdeTrayIcon")
#endif
	Q_INTERFACES(QtNote::QtNotePluginInterface QtNote::TrayInterface QtNote::DEIntegrationInterface QtNote::GlobalShortcutsInterface)
public:
	explicit KDEIntegration(QObject *parent = 0);

	virtual PluginMetadata metadata();
	bool init(Main *qtnote);

	TrayImpl* initTray(Main *qtnote);

    void activateWidget(QWidget *w);

	bool registerGlobalShortcut(const QString &id, const QKeySequence &key, QObject *receiver, const char *slot);
	
signals:
	
public slots:

private:
	QHash<QString, KAction*> _shortcuts;
};

} // namespace QtNote

#endif // KDEINTEGRATION_H
