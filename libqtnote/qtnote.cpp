#include <QAction>
#include <QApplication>
#include <QLibraryInfo>
#include <QTranslator>
#include <QLocale>
#include <QDesktopWidget>
#include <QStyle>
#include <QSettings>
#include <QMessageBox>
#include <QDebug>
#include <QProcess>
#include <QClipboard>
#include <QMenu>
#include <QDataStream>
#include <QDir>
#include <QSystemTrayIcon>
#include <QPluginLoader>
#include <QHBoxLayout>
#include <QDialog>
#ifdef Q_OS_MAC
# include <ApplicationServices/ApplicationServices.h>
#endif

#include "qtnote.h"
#include "notemanager.h"
#include "notedialog.h"
#include "notewidget.h"
#include "aboutdlg.h"
#include "optionsdlg.h"
#include "notemanagerdlg.h"
#include "utils.h"
#ifdef TOMBOY
#include "tomboystorage.h"
#endif
#include "ptfstorage.h"
#include "shortcutsmanager.h"
#include "pluginmanager.h"
#include "deintegrationinterface.h"
#include "trayimpl.h"
#include "globalshortcutsinterface.h"
#include "optionsplugins.h"

namespace QtNote {

class Main::Private : public QObject
{
	Q_OBJECT

public:
    Main *q;
    DEIntegrationInterface *de;
	TrayImpl *tray;
	GlobalShortcutsInterface *globalShortcuts;

    Private(Main *parent) :
        QObject(parent),
        q(parent),
		de(0),
		globalShortcuts(0)
	{

	}
};


Main::Main(QObject *parent) :
    QObject(parent),
    d(new Private(this)),
    _inited(false)
{
	// loading localization
	QString langFile = APPNAME;
	QTranslator *translator = new QTranslator(qApp);
	QTranslator *qtTranslator = new QTranslator(qApp);
	QStringList langDirs;
#ifdef Q_OS_UNIX
	langDirs << TRANSLATIONSDIR;
	QString qtLangDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#else
	QString appLangDir = qApp->applicationDirPath() + "/langs";
	langDirs << appLangDir;
	langDirs << QString("%1/langs").arg(QDir::currentPath());
	QString qtLangDir = appLangDir;
#endif
	QSettings settings;
	QString forcedLangName = settings.value("language").toString();
	bool autoLang = (forcedLangName.isEmpty() || forcedLangName == "auto");

	QLocale locale = autoLang? QLocale::system() : QLocale(forcedLangName);
	//qDebug() << forcedLangName;
	foreach (const QString &langDir, langDirs) {
		if (translator->load(locale, langFile, "_", langDir)) {
			qApp->installTranslator(translator);
			break;
		}
	}

	if (qtTranslator->load(locale, "qt", "_", qtLangDir)) {
		qApp->installTranslator(qtTranslator);
	}

	_pluginManager = new PluginManager(this);
	_pluginManager->loadPlugins();
	QString pluginError;
	// TODO load translations from plugins;
    if (!d->de) {
		pluginError = tr("Desktop integration plugin is not loaded");
	} else if (!d->tray) {
		pluginError = tr("Tray icon is not initialized");
    }

	if (!pluginError.isEmpty()) {
		QMessageBox::critical(0, tr("Initialization Error"), pluginError + "\n" + tr("Enable a plugin with required functionality and restart QtNote"));
		QDialog dlg;
		dlg.setLayout(new QHBoxLayout);
		dlg.layout()->addWidget(new OptionsPlugins(this));
		dlg.exec();
		return;
	}

	// itialzation of notes storages
	QList<NoteStorage*> storages;
	QStringList priorities = QSettings().value("storage.priority")
							 .toStringList();
	QStringList prioritiesR;
	while (priorities.count()) {
		prioritiesR.append(priorities.takeLast());
	}

#ifdef TOMBOY
	storages.append(new TomboyStorage(qApp));
#endif
	storages.append(new PTFStorage(qApp));

	while (storages.count()) {
		NoteStorage *storage = storages.takeFirst();
		if (storage->isAccessible()) {
			int priority = prioritiesR.indexOf(storage->systemName());
			NoteManager::instance()->registerStorage(storage,
													 priority >= 0? priority:0);
			connect(storage, SIGNAL(noteRemoved(NoteListItem)), SLOT(note_removed(NoteListItem)));
		} else {
			delete storage;
		}
	}
    _inited = NoteManager::instance()->loadAll();
	if (!NoteManager::instance()->loadAll()) {
		QMessageBox::critical(0, "QtNote", QObject::tr("no one of note "
							  "storages is accessible. can't continue.."));
		return;
	}

	_shortcutsManager = new ShortcutsManager(d->globalShortcuts, this);


	connect(d->tray, SIGNAL(exitTriggered()), SLOT(exitQtNote()));
	connect(d->tray, SIGNAL(newNoteTriggered()), SLOT(createNewNote()));
	connect(d->tray, SIGNAL(noteManagerTriggered()), SLOT(showNoteManager()));
	connect(d->tray, SIGNAL(optionsTriggered()), SLOT(showOptions()));
	connect(d->tray, SIGNAL(aboutTriggered()), SLOT(showAbout()));
	connect(d->tray, SIGNAL(showNoteTriggered(QString,QString)), SLOT(showNoteDialog(QString,QString)));

	_shortcutsManager->registerGlobal(ShortcutsManager::SKNoteFromSelection, this, SLOT(createNewNoteFromSelection()));
}

Main::~Main()
{

}

void Main::parseAppArguments(const QStringList &args)
{
	int i = 0;
	bool argsHandled = false;
	while (i < args.size()) {
		if (args.at(i) == QLatin1String("-n")) {
			if (i < args.size() + 1 && args.at(i + 1)[0] != '-') {
				i++;
				if (args.at(i) == QLatin1String("selection")) {
					createNewNoteFromSelection();
				}
			} else {
				createNewNote();
			}
			argsHandled = true;
		}
		i++;
	}
	QSettings s;
	if (!argsHandled && !s.value("first-start").toBool()) {
		QMessageBox *mb = new QMessageBox(QMessageBox::Information, tr("First Start"),
		  tr(
			  "This is your first start of QtNote note-taking application.\n\n"
			  "To start using just click on pencil in the system tray and choose \"New\" item to create new note.\n"
			  "Notes will be automatically saved to special storage, so you should not worry about this."
		  ), QMessageBox::Ok);
		mb->setModal(false);
		mb->setAttribute(Qt::WA_DeleteOnClose);
		mb->show();
		s.setValue("first-start", true);
	}
}

void Main::exitQtNote()
{
	QApplication::quit();
}

void Main::appMessageReceived(const QByteArray &msg)
{
	QDataStream stream(msg);
	QStringList params;
	stream >> params;
	parseAppArguments(params);
}

void Main::showAbout()
{
	AboutDlg *d = new AboutDlg;
	d->setAttribute(Qt::WA_DeleteOnClose);
	d->show();
	activateWidget(d);
}

void Main::showNoteManager()
{
	NoteManagerDlg *d = new NoteManagerDlg(this);
	connect(d, SIGNAL(showNoteRequested(QString,QString)), SLOT(showNoteDialog(QString,QString)));
	d->show();
	activateWidget(d);
}

void Main::showOptions()
{
	OptionsDlg *d = new OptionsDlg(this);
	d->setAttribute(Qt::WA_DeleteOnClose);
	d->show();
	activateWidget(d);
}

NoteWidget* Main::noteWidget(const QString &storageId, const QString &noteId)
{
	Note note;
	NoteWidget *w = new NoteWidget(storageId, noteId);
	w->setAcceptRichText(NoteManager::instance()->storage(storageId)
						   ->isRichTextAllowed());
	if (!noteId.isEmpty()) {
		note = NoteManager::instance()->getNote(storageId, noteId);
		if (note.isNull()) {
			qWarning("failed to load note: %s", qPrintable(noteId));
			return 0;
		}

		if (note.text().startsWith(note.title())) {
			w->setText(note.text());
		} else {
			w->setText(note.title() + "\n" + note.text());
		}
	}

	connect(w, SIGNAL(trashRequested()), SLOT(note_trashRequested()));
	connect(w, SIGNAL(saveRequested()), SLOT(note_saveRequested()));
	connect(w, SIGNAL(invalidated()), SLOT(note_invalidated()));
	return w;
}

void Main::showNoteDialog(const QString &storageId, const QString &noteId, const QString &contents)
{

	NoteDialog *dlg = 0;
	if (!noteId.isEmpty()) {
		// check if dialog for given storage and id is already opened
		dlg = NoteDialog::findDialog(storageId, noteId);
	}

	if (!dlg) {
		NoteWidget *w = noteWidget(storageId, noteId);
		if (!w) {
			return;
		}
		if (noteId.isEmpty() && !contents.isEmpty()) {
			w->setText(contents);
		}
		dlg = new NoteDialog(w);
	}
	dlg->show();
	activateWidget(dlg);
}

void Main::notifyError(const QString &text)
{
	d->tray->notifyError(text);
}

void Main::activateWidget(QWidget *w) const
{
	d->de->activateWidget(w);
	w->raise();
}

void Main::setTrayImpl(TrayImpl *tray)
{
	d->tray = tray;
}

void Main::setDesktopImpl(DEIntegrationInterface *de)
{
	d->de = de;
}

void Main::setGlobalShortcutsImpl(GlobalShortcutsInterface *gs)
{
	d->globalShortcuts = gs;
}

void Main::createNewNote()
{
	showNoteDialog(NoteManager::instance()->defaultStorage()->systemName());
}

void Main::createNewNoteFromSelection()
{
	QString contents;
#ifdef Q_OS_LINUX
	contents = QApplication::clipboard()->text(QClipboard::Selection);
#endif
#ifdef Q_OS_WIN
	int n = 0;
	QVector<INPUT> input(10);
	memset(input.data(), 0, input.size()*sizeof(INPUT));

	if (GetAsyncKeyState(VK_MENU) & (1 < 15)) {
		input[n].ki.dwFlags = KEYEVENTF_KEYUP;
		input[n].ki.wVk = VK_MENU;
		input[n++].ki.wScan = MapVirtualKey( VK_MENU, MAPVK_VK_TO_VSC );
	}
	if (GetAsyncKeyState(VK_SHIFT) & (1 < 15)) {
		input[n].ki.dwFlags = KEYEVENTF_KEYUP;
		input[n].ki.wVk = VK_SHIFT;
		input[n++].ki.wScan = MapVirtualKey(VK_SHIFT, MAPVK_VK_TO_VSC);
	}
	input[n].ki.wVk = VK_CONTROL;
	input[n].ki.dwFlags = 0;
	input[n++].ki.wScan = MapVirtualKey( VK_CONTROL, MAPVK_VK_TO_VSC );

	input[n].ki.wVk = 0x43; // Virtual key code for 'c'
	input[n].ki.dwFlags = 0;
	input[n++].ki.wScan = MapVirtualKey( 0x56, MAPVK_VK_TO_VSC );

	input[n].ki.dwFlags = KEYEVENTF_KEYUP;
	input[n].ki.wVk = input[0].ki.wVk;
	input[n++].ki.wScan = input[0].ki.wScan;

	input[n].ki.dwFlags = KEYEVENTF_KEYUP;
	input[n].ki.wVk = input[1].ki.wVk;
	input[n++].ki.wScan = input[1].ki.wScan;

	bool sent = true;
	for (int i = 0; i < n; i++) {
		input[i].type = INPUT_KEYBOARD;
		if(!SendInput(1, (LPINPUT)&(input[i]), sizeof(INPUT) ) ) {
			sent = false;
			break;
		}
		Sleep(30);
	}
	if (sent) {
		contents = QApplication::clipboard()->text();
	}
#endif
#ifdef Q_OS_MAC
	// copied from http://stackoverflow.com/questions/9758053/programming-with-qt-creator-and-cocoa-copying-the-selected-text-from-the-curre
	CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
	CGEventRef saveCommandDown = CGEventCreateKeyboardEvent(source, (CGKeyCode)8, true);
	CGEventSetFlags(saveCommandDown, kCGEventFlagMaskCommand);
	CGEventRef saveCommandUp = CGEventCreateKeyboardEvent(source, (CGKeyCode)8, false);

	CGEventPost(kCGAnnotatedSessionEventTap, saveCommandDown);
	CGEventPost(kCGAnnotatedSessionEventTap, saveCommandUp);

	CFRelease(saveCommandUp);
	CFRelease(saveCommandDown);
	CFRelease(source);

	contents = QApplication::clipboard()->text();
#endif
	if (contents.size()) {
		showNoteDialog(NoteManager::instance()->defaultStorage()->systemName(), QString(), contents);
	}
}

void Main::note_trashRequested()
{
	NoteWidget *nw = static_cast<NoteWidget *>(sender());
	NoteStorage *storage = NoteManager::instance()->storage(nw->storageId());
	if (!nw->noteId().isEmpty()) {
		storage->deleteNote(nw->noteId());
	}
}

void Main::note_saveRequested()
{
	NoteWidget *nw = static_cast<NoteWidget *>(sender());
	QString storageId = nw->storageId();
	QString noteId = nw->noteId();
	QString text = nw->text();
	NoteStorage *storage = NoteManager::instance()->storage(storageId);
	if (text.isEmpty()) { // delete empty note
		if (!noteId.isEmpty()) {
			storage->deleteNote(noteId);
		}
		return;
	}
	if (noteId.isEmpty()) {
		noteId = storage->createNote(text);
		nw->setNoteId(noteId);
	} else {
		if (NoteManager::instance()->getNote(storageId, noteId).text() != text) {
			storage->saveNote(noteId, text);
		}
	}
}

void Main::note_invalidated()
{
	NoteWidget *nw = static_cast<NoteWidget *>(sender());
	Note note = NoteManager::instance()->getNote(nw->storageId(), nw->noteId());
	if (!note.isNull() && nw->lastChangeElapsed() > note.lastChangeElapsed()) {
		if (note.text().startsWith(note.title())) {
			nw->setText(note.text());
		} else {
			nw->setText(note.title() + "\n" + note.text());
		}
	}
}

void Main::note_removed(const NoteListItem &noteItem)
{
	NoteDialog *dlg = NoteDialog::findDialog(noteItem.storageId, noteItem.id);
	if (dlg) {
		dlg->trashRequested();
	}
}

} // namespace QtNote

#include "qtnote.moc"
