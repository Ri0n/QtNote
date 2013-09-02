#include <KDE/KStatusNotifierItem>
#include <KDE/KWindowSystem>
#include <QWidget>
#include <QtPlugin>

#include "trayicon.h"

TrayIcon::TrayIcon(QObject *parent) :
	QObject(parent)
{
}

void TrayIcon::activateNote(QWidget *w)
{
    KWindowSystem::forceActiveWindow(w->winId());
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(kdeintegration, TrayIcon)
#endif
