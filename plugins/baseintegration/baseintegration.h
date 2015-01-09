#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>

#include "deintegrationinterface.h"
#include "trayinterface.h"
#include "qtnoteplugininterface.h"
#include "globalshortcutsinterface.h"

namespace QtNote {

class Main;

class BaseIntegration : public QObject, public QtNotePluginInterface, public DEIntegrationInterface,
		public TrayInterface, public GlobalShortcutsInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.BaseIntegration")
#endif
	Q_INTERFACES(QtNote::QtNotePluginInterface QtNote::DEIntegrationInterface QtNote::TrayInterface QtNote::GlobalShortcutsInterface)
public:
	explicit BaseIntegration(QObject *parent = 0);

	PluginMetadata metadata();
	bool init(Main *qtnote);
	void activateWidget(QWidget *w);
	TrayImpl* initTray(Main *qtnote);
	bool registerGlobalShortcut(const QKeySequence &key, QObject *receiver, const char *slot);

public slots:

private:
	Main *qtnote;
	TrayImpl *tray;
	
};

} // namespace QtNote

#endif // TRAYICON_H
