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
	void showNoteDialog(const QString &storageId, const QString &noteId = "");

private:
	QSystemTrayIcon *tray;
	QMenu *contextMenu;
	QAction *actQuit, *actNew;

private slots:
	void showNoteList(QSystemTrayIcon::ActivationReason);
	void exitQtNote();
	void createNewNote();
	void onSaveNote(const QString &storageId, const QString &noteId,
					const QString &text);
	void onDeleteNote(const QString &storageId, const QString &noteId);
};

#endif // WIDGET_H
