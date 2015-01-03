#include <QWidget>
#include <QtPlugin>
#include <QSystemTrayIcon>
#include <QAction>
#include <QMenu>
#include <QSettings>
#include <QStyle>
#include <QApplication>
#include <QDesktopWidget>

#include "baseintegration.h"
#include "qtnote.h"
#include "utils.h"


namespace QtNote {

class BaseIntegrationTray : public TrayImpl
{
	Q_OBJECT

	Main *qtnote;
	QSystemTrayIcon *tray;
	QMenu *contextMenu;
	QAction *actQuit, *actNew, *actAbout, *actOptions, *actManager;
public:
	BaseIntegrationTray(Main *qtnote, QObject *parent) :
		TrayImpl(parent),
		qtnote(qtnote)
	{
		actQuit = new QAction(QIcon(":/icons/exit"), tr("&Quit"), this);
		actNew = new QAction(QIcon(":/icons/new"), tr("&New"), this);
		actAbout = new QAction(QIcon(":/icons/trayicon"), tr("&About"), this);
		actOptions = new QAction(QIcon(":/icons/options"), tr("&Options"), this);
		actManager = new QAction(QIcon(":/icons/manager"), tr("&Note Manager"), this);

		contextMenu = new QMenu;
		contextMenu->addAction(actNew);
		contextMenu->addSeparator();
		contextMenu->addAction(actManager);
		contextMenu->addAction(actOptions);
		contextMenu->addAction(actAbout);
		contextMenu->addSeparator();
		contextMenu->addAction(actQuit);

		tray = new QSystemTrayIcon(this);
		tray->setIcon(QIcon(":/icons/trayicon"));
		tray->show();
		tray->setContextMenu(contextMenu);

		connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
				SLOT(showNoteList(QSystemTrayIcon::ActivationReason)));

		connect(actQuit, SIGNAL(triggered()), SIGNAL(exitTriggered()));
		connect(actNew, SIGNAL(triggered()), SIGNAL(newNoteTriggered()));
		connect(actManager, SIGNAL(triggered()), SIGNAL(noteManagerTriggered()));
		connect(actOptions, SIGNAL(triggered()), SIGNAL(optionsTriggered()));
		connect(actAbout, SIGNAL(triggered()), SIGNAL(aboutTriggered()));
	}

	void notifyError(const QString &message)
	{
		tray->showMessage(tr("Error"), message, QSystemTrayIcon::Warning, 5000);
	}

	void setNoteList(QList<NoteListItem>)
	{

	}

private slots:
	void showNoteList(QSystemTrayIcon::ActivationReason reason)
	{
		if (reason == QSystemTrayIcon::MiddleClick || reason == QSystemTrayIcon::DoubleClick) {
			emit newNoteTriggered();
			return;
		}
		if (reason != QSystemTrayIcon::Trigger) {
			return;
		}
		QMenu menu;
		menu.addAction(actNew);
		menu.addSeparator();
		QSettings s;
		QList<NoteListItem> notes = NoteManager::instance()->noteList(
									s.value("ui.menu-notes-amount", 15).toInt());
		for (int i=0; i<notes.count(); i++) {
			menu.addAction(menu.style()->standardIcon(
				QStyle::SP_MessageBoxInformation),
				Utils::cuttedDots(notes[i].title, 48).replace('&', "&&")
			)->setData(i);
		}
		menu.show();
		qtnote->activateWidget(&menu);
		QRect dr = QApplication::desktop()->availableGeometry(QCursor::pos());
		QRect ir = tray->geometry();
		QRect mr = menu.geometry();
		if (ir.isEmpty()) { // O_O but with kde this happens...
			ir = QRect(QCursor::pos() - QPoint(8,8), QSize(16,16));
		}
		mr.setSize(menu.sizeHint());
		if (ir.left() < dr.width()/2) {
			if (ir.top() < dr.height()/2) {
				mr.moveTopLeft(ir.bottomLeft());
			} else {
				mr.moveBottomLeft(ir.topLeft());
			}
		} else {
			if (ir.top() < dr.height()/2) {
				mr.moveTopRight(ir.bottomRight());
			} else {
				mr.moveBottomRight(ir.topRight());
			}
		}
		// and now align to available desktop geometry
		if (mr.right() > dr.right()) {
			mr.moveRight(dr.right());
		}
		if (mr.bottom() > dr.bottom()) {
			mr.moveBottom(dr.bottom());
		}
		if (mr.left() < dr.left()) {
			mr.moveLeft(dr.left());
		}
		if (mr.top() < dr.top()) {
			mr.moveTop(dr.top());
		}
		QAction *act = menu.exec(mr.topLeft());

		if (act && act != actNew) {
			NoteListItem &note = notes[act->data().toInt()];
			emit showNoteTriggered(note.storageId, note.id);
		}
	}
};

class BaseIntegration::Private : public QObject
{
    Q_OBJECT

public:
    Main *qtnote;
    BaseIntegration *q;
	BaseIntegrationTray *tray;

    Private(BaseIntegration *q) :
        QObject(q),
		q(q),
		tray(0)
    {

    }
};


//******************************************
// BaseIntegration
//******************************************
BaseIntegration::BaseIntegration(QObject *parent) :
    QObject(parent),
    d(new Private(this))
{
}

PluginMetadata BaseIntegration::metadata()
{
	PluginMetadata md;
	md.pluginType = PluginMetadata::DEIntegration;
	md.icon = QIcon(":/icons/logo");
	md.name = "Base Integration";
	md.description = "Provides fallback desktop environment integration";
	md.author = "Sergey Il'inykh <rion4ik@gmail.com>";
	md.version = 0x010000;	// plugin's version 0xXXYYZZPP
	md.minVersion = 0x020301; // minimum compatible version of QtNote
	md.maxVersion = 0x030000; // maximum compatible version of QtNote
	//md.extra.insert("de", QStringList() << "KDE-4");
	return md;
}

bool BaseIntegration::init(Main *qtnote)
{
	d->qtnote = qtnote;
	return true;
}

void BaseIntegration::activateWidget(QWidget *w)
{
    w->activateWindow();
	w->raise();
}

TrayImpl* BaseIntegration::initTray(Main *qtnote)
{
	d->tray = new BaseIntegrationTray(qtnote, this);
	return d->tray;
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(kdeintegration, QtNote::BaseIntegration)
#endif
