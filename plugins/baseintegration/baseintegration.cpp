#include <QWidget>
#include <QtPlugin>
#include <QSystemTrayIcon>

#include "baseintegration.h"
#include "qtnote.h"

namespace QtNote {

class BaseIntegration::Private : public QObject
{
    Q_OBJECT

public:
    Main *qtnote;
    BaseIntegration *q;

    Private(BaseIntegration *q) :
        QObject(q),
        q(q)
    {

    }

private slots:
    void showNoteList(QSystemTrayIcon::ActivationReason)
    {
        if (reason == QSystemTrayIcon::MiddleClick || reason == QSystemTrayIcon::DoubleClick) {
            createNewNote();
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
        q->activateWidget(menu);
        QRect dr = QApplication::desktop()->availableGeometry(QCursor::pos());
        QRect ir = d->tray->geometry();
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
            showNoteDialog(note.storageId, note.id);
        }
    }
};

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
    // TODO isn't it better to try few dynamic_casts right in qtnote.
    qtnote->setTrayImplementation(this);
    qtnote->setDesktopImplementation(this);
	return true;
}

void BaseIntegration::activateWidget(QWidget *w)
{
    w->activateWindow();
    w->raise();
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(kdeintegration, QtNote::BaseIntegration)
#endif
