#ifndef WIDGET_H
#define WIDGET_H

#include <QtGui/QWidget>
#include <QSystemTrayIcon>
#include <QAction>
#include <QMenu>

#include "tomboynote.h"

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();

private:
	QSystemTrayIcon *tray;
	QMenu *contextMenu;
	QAction *actQuit, *actNew;
	QList<TomboyNote *> notes;

private slots:
	void showNoteList(QSystemTrayIcon::ActivationReason);
	void exitQtNote();
	void createNewNote();
};

#endif // WIDGET_H
