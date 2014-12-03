#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>

#include "deintegrationinterface.h"
#include "qtnoteplugininterface.h"

namespace QtNote {

class TrayIcon : public QObject, public QtNotePluginInterface, public TrayHandlerInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.KdeTrayIcon")
#endif
	Q_INTERFACES(QtNote::QtNotePluginInterface QtNote::TrayHandlerInterface)
public:
	explicit TrayIcon(QObject *parent = 0);

	virtual PluginMetadata metadata();
	bool init(Main *qtnote);

	void setNoteList(QList<NoteListItem>);
	void notify(const QString &message);
	
signals:
	
public slots:
	
};

} // namespace QtNote

#endif // TRAYICON_H
