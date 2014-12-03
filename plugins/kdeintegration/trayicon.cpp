#include <KDE/KStatusNotifierItem>
#include <KDE/KWindowSystem>
#include <QWidget>
#include <QtPlugin>

#include "trayicon.h"

namespace QtNote {

TrayIcon::TrayIcon(QObject *parent) :
	QObject(parent)
{
}

PluginMetadata TrayIcon::metadata()
{
	PluginMetadata md;
	md.pluginType = PluginMetadata::DEIntegration;
	md.icon = QIcon(":/icons/logo");
	md.name = "KDE Integration";
	md.description = "Provide native look and feel for KDE users";
	md.author = "Sergey Il'inykh <rion4ik@gmail.com>";
	md.version = 0x010000;	// plugin's version 0xXXYYZZPP
	md.minVersion = 0x020300; // minimum compatible version of QtNote
	md.maxVersion = 0x030000; // maximum compatible version of QtNote
	md.extra.insert("de", QStringList() << "KDE-4");
	return md;
}

bool TrayIcon::init(Main *qtnote)
{
	// TODO set trayicon to QtNote class
	return true;
}

void TrayIcon::setNoteList(QList<NoteListItem>)
{

}

void TrayIcon::notify(const QString &message)
{

}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(kdeintegration, QtNote::TrayIcon)
#endif
