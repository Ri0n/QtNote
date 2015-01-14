#ifndef BASEINTEGRATIONTRAY_H
#define BASEINTEGRATIONTRAY_H

#include <QSystemTrayIcon>

#include "trayimpl.h"
#include "qtnote.h"

class QMenu;
class QAction;

namespace QtNote {

class BaseIntegrationTray : public TrayImpl
{
	Q_OBJECT

	Main *qtnote;
	QSystemTrayIcon *tray;
	QMenu *contextMenu;
	QAction *actQuit, *actNew, *actAbout, *actOptions, *actManager;

public:
	explicit BaseIntegrationTray(Main *qtnote, QObject *parent = 0);
	void notifyError(const QString &message);

signals:

public slots:

private slots:
	void showNoteList(QSystemTrayIcon::ActivationReason reason);

};

} // namespace QtNote

#endif // BASEINTEGRATIONTRAY_H
