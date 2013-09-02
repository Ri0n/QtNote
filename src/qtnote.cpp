#include <QAction>
#include <QApplication>
#include <QLibraryInfo>
#include <QTranslator>
#include <QLocale>
#include <QDesktopWidget>
#include <QStyle>
#include <QSettings>
#include <QMessageBox>
#include <QtSingleApplication>
#include <QDebug>
#include <QProcess>
#include <QClipboard>
#include <QMenu>
#include <QDataStream>
#include <QDir>
#include <QPluginLoader>
#ifdef Q_OS_MAC
# include <ApplicationServices/ApplicationServices.h>
#endif
#if defined(Q_OS_WIN) && QT_VERSION < 0x040800
# include <winnls.h>
#  ifndef LOCALE_SNAME
#   define LOCALE_SNAME 0x5c
#  endif
# include <QLibrary>
# include <QSysInfo>
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
#include "../plugins/trayiconinterface.h"

#if QT_VERSION < 0x040800
static QLocale systemUILocale()
{
#ifdef Q_OS_WIN
	QByteArray buffer;
	buffer.resize(512);
	LCID lcid = MAKELCID(GetUserDefaultUILanguage(), SORT_DEFAULT);
	bool success = false;
	if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA && QSysInfo::windowsVersion() < QSysInfo::WV_NT_based) {
		success = GetLocaleInfo(lcid,
								LOCALE_SNAME, (wchar_t*)buffer.data(),
								buffer.size() / sizeof(wchar_t)) > 0;
	} else {
		typedef HRESULT (*LcidToRfc1766)(LCID Locale, LPTSTR pszRfc1766, int nChar);
		LcidToRfc1766 pLcidToRfc1766;
		QLibrary mlang("mlang");
		pLcidToRfc1766 = (LcidToRfc1766) mlang.resolve("LcidToRfc1766W");
		if (pLcidToRfc1766) {
			success = pLcidToRfc1766(lcid, (wchar_t*)buffer.data(), buffer.size() / sizeof(wchar_t)) == S_OK;
		}
		mlang.unload();
	}
	if (success) {
		QString name = QString::fromWCharArray((wchar_t*)buffer.data());
		name = name.section('-', 0, 0) + "_" + name.section('-', 1, 1).toUpper();
		return QLocale(name);
	} else {
		qErrnoWarning("systemUILocale");
	}
	return QLocale();
#else
	return QLocale::system();
#endif
}
#endif

QtNote::QtNote(QObject *parent) :
	QObject(parent),
	pluginTrayIcon(0)
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
#if QT_VERSION >= 0x040800
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
#else
	QString locale = autoLang? systemUILocale().name() : forcedLangName;
	langFile = QString("%1_%2").arg(langFile, locale);
	foreach (const QString &langDir, langDirs) {
		if (translator->load(langFile, langDir)) {
			qApp->installTranslator(translator);
			break;
		}
	}

	if (qtTranslator->load("qt_" + locale, qtLangDir)) {
		qApp->installTranslator(qtTranslator);
	}
#endif

	QDir pluginsDir = QDir(qApp->applicationDirPath());
#if defined(Q_OS_WIN)
	if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
		pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
	if (pluginsDir.dirName() == "MacOS") {
		pluginsDir.cdUp();
		pluginsDir.cdUp();
		pluginsDir.cdUp();
	}
#endif
	if (pluginsDir.dirName() == "src") {
		pluginsDir.cdUp();
	}
	pluginsDir.cd("plugins");
	foreach (QString dirName, pluginsDir.entryList(QDir::Dirs)) {
		QDir pluginDir = pluginsDir;
		pluginDir.cd(dirName);
		foreach (QString fileName, pluginDir.entryList(QDir::Files)) {
			 QPluginLoader loader(pluginDir.absoluteFilePath(fileName));
			 QObject *plugin = loader.instance();
			 if (plugin) {
				 if (!pluginTrayIcon && (pluginTrayIcon = qobject_cast<TrayIconInterface*>(plugin))) {
					 continue;
				 }
			 }
		 }
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
		} else {
			delete storage;
		}
	}
	inited_ = NoteManager::instance()->loadAll();
	if (!NoteManager::instance()->loadAll()) {
		QMessageBox::critical(0, "QtNote", QObject::tr("no one of note "
							  "storages is accessible. can't continue.."));
		return;
	}


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

	_shortcutsManager = new ShortcutsManager(this);

	connect(QtSingleApplication::instance(), SIGNAL(messageReceived(const QByteArray &)), SLOT(appMessageReceived(const QByteArray &)));
	connect(actQuit, SIGNAL(triggered()), this, SLOT(exitQtNote()));
	connect(actNew, SIGNAL(triggered()), this, SLOT(createNewNote()));
	connect(actManager, SIGNAL(triggered()), this, SLOT(showNoteManager()));
	connect(actOptions, SIGNAL(triggered()), this, SLOT(showOptions()));
	connect(actAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
	connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
			SLOT(showNoteList(QSystemTrayIcon::ActivationReason)));
	connect(_shortcutsManager->shortcut(QLatin1String("note-from-selection")), SIGNAL(triggered()), SLOT(createNewNoteFromSelection()));

	parseAppArguments(QtSingleApplication::instance()->arguments().mid(1));
}

void QtNote::parseAppArguments(const QStringList &args)
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

void QtNote::exitQtNote()
{
	QApplication::quit();
}

void QtNote::appMessageReceived(const QByteArray &msg)
{
	QDataStream stream(msg);
	QStringList params;
	stream >> params;
	parseAppArguments(params);
}

void QtNote::showAbout()
{
	AboutDlg *d = new AboutDlg;
	d->setAttribute(Qt::WA_DeleteOnClose);
	d->show();
}

void QtNote::showNoteManager()
{
	NoteManagerDlg *d = new NoteManagerDlg(this);
	connect(d, SIGNAL(showNoteRequested(QString,QString)), SLOT(showNoteDialog(QString,QString)));
	d->show();
}

void QtNote::showOptions()
{
	OptionsDlg *d = new OptionsDlg(this);
	d->setAttribute(Qt::WA_DeleteOnClose);
	d->show();
}

NoteWidget* QtNote::noteWidget(const QString &storageId, const QString &noteId)
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

void QtNote::showNoteDialog(const QString &storageId, const QString &noteId, const QString &contents)
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
	dlg->activateWindow();
	dlg->raise();
	if (pluginTrayIcon) {
		pluginTrayIcon->activateNote(dlg);
	}
}

void QtNote::notifyError(const QString &text)
{
	tray->showMessage(tr("Error"), text, QSystemTrayIcon::Warning, 5000);
}

void QtNote::showNoteList(QSystemTrayIcon::ActivationReason reason)
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
	menu.activateWindow();
	menu.raise();
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
		showNoteDialog(note.storageId, note.id);
	}
}

void QtNote::createNewNote()
{
	showNoteDialog(NoteManager::instance()->defaultStorage()->systemName());
}

void QtNote::createNewNoteFromSelection()
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

void QtNote::note_trashRequested()
{
	NoteWidget *nw = static_cast<NoteWidget *>(sender());
	NoteStorage *storage = NoteManager::instance()->storage(nw->storageId());
	storage->deleteNote(nw->noteId());
}

void QtNote::note_saveRequested()
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
		if (NoteManager::instance()->getNote(storageId, noteId).text() != text)
		{
			storage->saveNote(noteId, text);
		}
	}
}

void QtNote::note_invalidated()
{
	NoteWidget *nw = static_cast<NoteWidget *>(sender());
	Note note = NoteManager::instance()->getNote(storageId, noteId);
	if (!note.isNull() && nw->lastChangeTime() < note.lastChange()) {
		// todo update text
	}
}
