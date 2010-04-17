#include <QtGui/QApplication>
#include <QMessageBox>
#include "mainwidget.h"
#include "notemanager.h"
#include "tomboystorage.h"
#include "qtnoteptfstorage.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


	Q_INIT_RESOURCE(main);

	if (!QSystemTrayIcon::isSystemTrayAvailable()) {
		QMessageBox::critical(0, QObject::tr("QNote"),
							  QObject::tr("I couldn't detect any system tray "
										   "on this system."));
		return 1;
	}
	QApplication::setQuitOnLastWindowClosed(false);

	QList<NoteStorage*> storages;
	storages.append(new TomboyStorage(&a));
	storages.append(new QtNotePTFStorage(&a));

	while (storages.count()) {
		NoteStorage *storage = storages.takeFirst();
		if (storage->isAccessible()) {
			NoteManager::instance()->registerStorage(storage, true);
		} else {
			delete storage;
		}
	}
	if (!NoteManager::instance()->loadAll()) {
		qWarning("no one of note storages reported success. can't continue..");
		return 1;
	}

	Widget w;
    return a.exec();
}
