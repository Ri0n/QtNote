#ifndef UBUNTUTRAY_H
#define UBUNTUTRAY_H

#include "trayimpl.h"

class QAction;
class QSystemTrayIcon;
class QTimer;
class QMenu;

namespace QtNote {

class Main;

class UbuntuTray : public TrayImpl
{
	Q_OBJECT
public:
	explicit UbuntuTray(Main *qtnote, QObject *parent);
	~UbuntuTray();
	void notifyError(const QString &message);

signals:

public slots:

private slots:
	void rebuildMenu();
	void noteSelected();

private:
	Main *qtnote;
	QSystemTrayIcon *sti;
	QAction *actQuit, *actNew, *actAbout, *actOptions, *actManager;
	QTimer *menuUpdateTimer;
	QMenu *contextMenu;
	uint menuUpdateHash;
};

}

#endif // UBUNTUTRAY_H
