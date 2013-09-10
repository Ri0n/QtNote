#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>

#include "deintegrationinterface.h"
#include "qtnoteplugininterface.h"

class BaseIntegration : public QObject, public QtNotePluginInterface, public DEIntegrationInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.BaseIntegration")
#endif
	Q_INTERFACES(QtNotePluginInterface BaseIntegrationInterface)
public:
	explicit BaseIntegration(QObject *parent = 0);

	virtual PluginMetadata metadata();
	bool init();

	void activateNote(QWidget *w);
	
signals:
	
public slots:
	
};

#endif // TRAYICON_H
