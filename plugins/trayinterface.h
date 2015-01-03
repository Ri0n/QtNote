#ifndef TRAYINTERFACE_H
#define TRAYINTERFACE_H

#include <QObject>

#include "notemanager.h"

namespace QtNote {

class Main;

class TrayImpl : public QObject
{
	Q_OBJECT

public:
	inline TrayImpl(QObject *parent = 0) : QObject(parent) {}
	virtual void notifyError(const QString &message) = 0;
	virtual void setNoteList(QList<NoteListItem>) = 0;

signals:
	void exitTriggered();
	void newNoteTriggered();
	void noteManagerTriggered();
	void optionsTriggered();
	void aboutTriggered();
	void showNoteTriggered(const QString &storageId, const QString &noteId);
};

class TrayInterface
{
public:
	virtual TrayImpl* initTray(Main *qtnote) = 0;
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::TrayInterface,
					 "com.rion-soft.QtNote.TrayInterface/1.0")

#endif // TRAYINTERFACE_H
