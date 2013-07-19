#ifndef QTNOTE_H
#define QTNOTE_H

#include <QObject>
#include <QSystemTrayIcon>

class QAction;

class QtNote : public QObject
{
	Q_OBJECT
public:
	explicit QtNote(QObject *parent = 0);
	inline bool isOperable() const { return inited_; }

public slots:
	void showNoteDialog(const QString &storageId, const QString &noteId = QString::null, const QString &contents = QString::null);

private:
	bool inited_;
	QSystemTrayIcon *tray;
	QMenu *contextMenu;
	QAction *actQuit, *actNew, *actAbout, *actOptions, *actManager;

	void parseAppArguments(const QStringList &args);
private slots:
	void showNoteList(QSystemTrayIcon::ActivationReason);
	void exitQtNote();
	void showAbout();
	void showNoteManager();
	void showOptions();
	void createNewNote();
	void createNewNoteFromSelection();
	void onSaveNote();
	void onDeleteNote();
	void appMessageReceived(const QByteArray &msg);
	
};

#endif // QTNOTE_H
