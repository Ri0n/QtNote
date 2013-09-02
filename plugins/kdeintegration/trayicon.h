#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>

#include "../trayiconinterface.h"

class TrayIcon : public QObject, public TrayIconInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.KdeTrayIcon")
#endif
	Q_INTERFACES(TrayIconInterface)
public:
	explicit TrayIcon(QObject *parent = 0);

	void activateNote(QWidget *w);
	
signals:
	
public slots:
	
};

#endif // TRAYICON_H
