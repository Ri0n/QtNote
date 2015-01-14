#ifndef KDEINTEGRATIONTRAY_H
#define KDEINTEGRATIONTRAY_H

#include "trayimpl.h"

class QAction;
class KStatusNotifierItem;

namespace QtNote {

class Main;

class KDEIntegrationTray : public TrayImpl
{
	Q_OBJECT
public:
	explicit KDEIntegrationTray(Main *qtnote, QObject *parent);
	void notifyError(const QString &message);

signals:

public slots:

private:
	Main *qtnote;
	KStatusNotifierItem *sni;
};

}

#endif // KDEINTEGRATIONTRAY_H
