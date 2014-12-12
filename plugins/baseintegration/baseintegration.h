#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>

#include "deintegrationinterface.h"
#include "qtnoteplugininterface.h"

namespace QtNote {

class BaseIntegration : public QObject, public QtNotePluginInterface, public DEIntegrationInterface, public TrayHandlerInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.BaseIntegration")
#endif
    Q_INTERFACES(QtNote::QtNotePluginInterface QtNote::DEIntegrationInterface, QtNote::TrayHandlerInterface)
public:
	explicit BaseIntegration(QObject *parent = 0);

	virtual PluginMetadata metadata();
	bool init(Main *qtnote);

	void activateWidget(QWidget *w);
	
signals:
	
public slots:

private:
    class Private;
    Private *d;
	
};

} // namespace QtNote

#endif // TRAYICON_H
