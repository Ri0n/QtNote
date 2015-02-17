#ifndef UBUNTUPLUGIN_H
#define UBUNTUPLUGIN_H

#include <QObject>

#include "qtnoteplugininterface.h"
#include "trayinterface.h"
#include "deintegrationinterface.h"

namespace QtNote {

class UbuntuPlugin : public QObject, public PluginInterface, public TrayInterface,
		DEIntegrationInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.Ubuntu")
#endif
	Q_INTERFACES(QtNote::PluginInterface QtNote::TrayInterface QtNote::DEIntegrationInterface)
public:
	explicit UbuntuPlugin(QObject *parent = 0);

	virtual PluginMetadata metadata();

	TrayImpl* initTray(Main *qtnote);

	void activateWidget(QWidget *w);

private slots:
	void activator();

private:
};

} // namespace QtNote

#endif // UBUNTUPLUGIN_H
