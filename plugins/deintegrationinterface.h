#ifndef DEINTEGRATIONINTERFACE_H
#define DEINTEGRATIONINTERFACE_H

#include "notestorage.h"

class QWidget;

namespace QtNote {

class TrayHandlerInterface
{
public:
	virtual void setNoteList(QList<NoteListItem>) = 0;
	virtual void notify(const QString &message) = 0;
};

class DEIntegrationInterface
{
public:
    virtual void activateWidget(QWidget *w) = 0;
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::TrayHandlerInterface,
					 "com.rion-soft.QtNote.TrayHandlerInterface/1.0")

Q_DECLARE_INTERFACE(QtNote::DEIntegrationInterface,
					 "com.rion-soft.QtNote.DEIntegrationInterface/1.0")

#endif
