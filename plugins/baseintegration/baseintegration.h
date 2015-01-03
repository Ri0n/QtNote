#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>

#include "deintegrationinterface.h"
#include "trayinterface.h"
#include "qtnoteplugininterface.h"

namespace QtNote {

class Main;

class BaseIntegration : public QObject, public QtNotePluginInterface, public DEIntegrationInterface, public TrayInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.BaseIntegration")
#endif
	Q_INTERFACES(QtNote::QtNotePluginInterface QtNote::DEIntegrationInterface QtNote::TrayInterface)
public:
	explicit BaseIntegration(QObject *parent = 0);

	virtual PluginMetadata metadata();
	bool init(Main *qtnote);

	void activateWidget(QWidget *w);

	TrayImpl* initTray(Main *qtnote);

public slots:

private:
    class Private;
    Private *d;
	
};

} // namespace QtNote

#endif // TRAYICON_H
