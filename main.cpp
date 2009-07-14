#include <QtGui/QApplication>
#include <QMessageBox>
#include "mainwidget.h"

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

	Widget w;
    return a.exec();
}
