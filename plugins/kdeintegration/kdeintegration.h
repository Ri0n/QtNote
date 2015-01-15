#ifndef KDEINTEGRATION_H
#define KDEINTEGRATION_H

#include <QObject>

#include "deintegrationinterface.h"
#include "qtnoteplugininterface.h"
#include "trayinterface.h"

namespace QtNote {

class KDEIntegration : public QObject, public QtNotePluginInterface, public TrayInterface, public DEIntegrationInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.KdeTrayIcon")
#endif
	Q_INTERFACES(QtNote::QtNotePluginInterface QtNote::TrayInterface QtNote::DEIntegrationInterface)
public:
	explicit KDEIntegration(QObject *parent = 0);

	virtual PluginMetadata metadata();
	bool init(Main *qtnote);

	TrayImpl* initTray(Main *qtnote);

    void activateWidget(QWidget *w);
	
signals:
	
public slots:
	
};

} // namespace QtNote

#endif // KDEINTEGRATION_H
