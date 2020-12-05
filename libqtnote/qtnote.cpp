#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDataStream>
#include <QDesktopWidget>
#include <QDialog>
#include <QDir>
#include <QHBoxLayout>
#include <QLibraryInfo>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPluginLoader>
#include <QProcess>
#include <QSettings>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTranslator>
#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "aboutdlg.h"
#include "deintegrationinterface.h"
#include "globalshortcutsinterface.h"
#include "notedialog.h"
#include "notemanager.h"
#include "notemanagerdlg.h"
#include "notewidget.h"
#include "notificationinterface.h"
#include "optionsdlg.h"
#include "optionsplugins.h"
#include "pluginmanager.h"
#include "ptfstorage.h"
#include "qtnote.h"
#include "shortcutsmanager.h"
#include "trayimpl.h"
#include "utils.h"

//#define MAIN_DEBUG
#ifdef MAIN_DEBUG
#include <QDebug>
#endif

void initResources() { Q_INIT_RESOURCE(main); }

namespace QtNote {

class Main::Private : public QObject {
    Q_OBJECT

public:
    Main *                    q;
    DEIntegrationInterface *  de;
    TrayImpl *                tray;
    GlobalShortcutsInterface *globalShortcuts;
    NotificationInterface *   notifier;

    Private(Main *parent) : QObject(parent), q(parent), de(0), globalShortcuts(0), notifier(0) { }
};

Main::Main(QObject *parent) : QObject(parent), d(new Private(this)), _inited(false)
{
    // loading localization
    QString      langFile     = APPNAME;
    QTranslator *translator   = new QTranslator(qApp);
    QTranslator *qtTranslator = new QTranslator(qApp);
    QStringList  langDirs;
    QStringList  qtLangDirs;
    QString      dlTrDir = Utils::qtnoteDataDir() + QLatin1String("/langs"); // where translaations could be downloaded

#if defined(DEVEL) || defined(Q_OS_UNIX)
    langDirs << TRANSLATIONSDIR << dlTrDir; // in devel mode TRANSLATIONSDIR will refer to source/langs
    qtLangDirs << QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#else
    langDirs << qApp->applicationDirPath() + QLatin1String("/langs") << dlTrDir;
    qtLangDirs << langDirs;
#endif

    QSettings settings;
    QString   forcedLangName = settings.value(QLatin1String("language")).toString();
    bool      autoLang       = (forcedLangName.isEmpty() || forcedLangName == "auto");

    QLocale locale = autoLang ? QLocale::system() : QLocale(forcedLangName);
    // qDebug() << forcedLangName;
    foreach (const QString &langDir, langDirs) {
        if (translator->load(locale, langFile, "_", langDir)) {
            qApp->installTranslator(translator);
            break;
        }
    }

    foreach (const QString &langDir, qtLangDirs) {
        if (qtTranslator->load(locale, "qt", "_", langDir)) {
            qApp->installTranslator(qtTranslator);
            break;
        }
    }

    initResources();

    _pluginManager = new PluginManager(this);
    _pluginManager->loadPlugins();
    QString pluginError;
    // TODO load translations from plugins;
    if (!d->de) {
        pluginError = tr("Desktop integration plugin is not loaded");
    } else if (!d->tray) {
        pluginError = tr("Tray icon is not initialized");
    } else if (!d->notifier) {
        pluginError = tr("Notifications plugin is not loaded");
    }

    if (!pluginError.isEmpty()) {
        QMessageBox::critical(0, tr("Initialization Error"),
                              pluginError + "\n"
                                  + tr("Enable a plugin with required functionality and restart QtNote"));
        QDialog dlg;
        dlg.setLayout(new QHBoxLayout);
        dlg.layout()->addWidget(new OptionsPlugins(this));
        dlg.exec();
        return;
    }

    auto ptfStorage = NoteStorage::Ptr(new PTFStorage());
    registerStorage(ptfStorage);

    _inited = NoteManager::instance()->loadAll();
    if (!NoteManager::instance()->loadAll()) {
        QMessageBox::critical(0, "QtNote",
                              QObject::tr("no one of note "
                                          "storages is accessible. can't continue.."));
        return; // TODO review removing this
    }
    NoteManager::instance()->setPriorities(settings.value("storage.priority", QStringList()).toStringList());

    _shortcutsManager = new ShortcutsManager(d->globalShortcuts, this);

    connect(d->tray, SIGNAL(exitTriggered()), SLOT(exitQtNote()));
    connect(d->tray, SIGNAL(newNoteTriggered()), SLOT(createNewNote()));
    connect(d->tray, SIGNAL(noteManagerTriggered()), SLOT(showNoteManager()));
    connect(d->tray, SIGNAL(optionsTriggered()), SLOT(showOptions()));
    connect(d->tray, SIGNAL(aboutTriggered()), SLOT(showAbout()));
    connect(d->tray, SIGNAL(showNoteTriggered(QString, QString)), SLOT(showNoteDialog(QString, QString)));

    // TODO it's a little ugly. refactor
    QAction *actNoteFromSel = new QAction(_shortcutsManager->friendlyName(ShortcutsManager::SKNoteFromSelection), this);
    connect(actNoteFromSel, SIGNAL(triggered(bool)), SLOT(createNewNoteFromSelection()));
    _shortcutsManager->registerGlobal(ShortcutsManager::SKNoteFromSelection, actNoteFromSel);
}

Main::~Main() { }

void Main::parseAppArguments(const QStringList &args)
{
    int  i           = 0;
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
        QMessageBox *mb = new QMessageBox(
            QMessageBox::Information, tr("First Start"),
            tr("This is your first start of QtNote note-taking application.\n\n"
               "To start using just click on pencil in the system tray and choose \"New\" item to create new note.\n"
               "Notes will be automatically saved to special storage, so you should not worry about this."),
            QMessageBox::Ok);
        mb->setModal(false);
        mb->setAttribute(Qt::WA_DeleteOnClose);
        mb->show();
        s.setValue("first-start", true);
    }
}

void Main::exitQtNote() { QApplication::quit(); }

void Main::appMessageReceived(const QString &msg) { parseAppArguments(msg.split(QLatin1String("!qtnote_argdelim!"))); }

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
    connect(d, SIGNAL(showNoteRequested(QString, QString)), SLOT(showNoteDialog(QString, QString)));
    d->show();
    activateWidget(d);
}

void Main::showOptions()
{
    OptionsDlg *d = new OptionsDlg(this);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();
    connect(d, SIGNAL(accepted()), SIGNAL(settingsUpdated()));
    activateWidget(d);
}

NoteWidget *Main::noteWidget(const QString &storageId, const QString &noteId, const QString &contents)
{
    Note note;
    if (!noteId.isEmpty()) {
        note = NoteManager::instance()->note(storageId, noteId);
        if (note.isNull()) {
            qWarning("failed to load note: %s", qPrintable(noteId));
            return 0;
        }
    }

    NoteWidget *w = new NoteWidget(storageId, noteId);
    w->setAcceptRichText(NoteManager::instance()->storage(storageId)->isRichTextAllowed());
    emit noteWidgetCreated(w);
    if (noteId.isEmpty()) {
        if (!contents.isEmpty()) {
            w->setText(contents);
        }
    } else {
#ifdef MAIN_DEBUG
        qDebug() << "Main::noteWidget";
#endif
        if (note.text().startsWith(note.title())) {
            w->setText(note.text());
        } else {
            w->setText(note.title() + "\n" + note.text());
        }
    }

    connect(this, SIGNAL(settingsUpdated()), w, SLOT(rereadSettings()));
    connect(w, SIGNAL(trashRequested()), SLOT(note_trashRequested()));
    connect(w, SIGNAL(saveRequested()), SLOT(note_saveRequested()));
    connect(w, SIGNAL(invalidated()), SLOT(note_invalidated()));
    emit noteWidgetInitiated(w);
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
        auto nw = noteWidget(storageId, noteId, contents);
        if (!nw) {
            return;
        }
        dlg = new NoteDialog(nw);
        dlg->setWindowIcon(NoteManager::instance()->storage(storageId)->noteIcon());
    }
    dlg->show();
    activateWidget(dlg);
}

void Main::notifyError(const QString &text) { d->notifier->notifyError(text); }

void Main::activateWidget(QWidget *w) const { d->de->activateWidget(w); }

void Main::setTrayImpl(TrayImpl *tray) { d->tray = tray; }

void Main::setDesktopImpl(DEIntegrationInterface *de) { d->de = de; }

void Main::setGlobalShortcutsImpl(GlobalShortcutsInterface *gs) { d->globalShortcuts = gs; }

void Main::setNotificationImpl(NotificationInterface *notifier) { d->notifier = notifier; }

void Main::registerStorage(NoteStorage::Ptr &storage)
{
    NoteManager::instance()->registerStorage(storage);
    connect(storage.data(), SIGNAL(noteRemoved(NoteListItem)), SLOT(note_removed(NoteListItem)));
    connect(storage.data(), SIGNAL(storageErorr(QString)), SLOT(notifyError(QString)));
}

void Main::unregisterStorage(NoteStorage::Ptr &storage)
{
    if (storage) {
        NoteManager::instance()->unregisterStorage(storage);
    }
}

void Main::createNewNote() { showNoteDialog(NoteManager::instance()->defaultStorage()->systemName()); }

void Main::createNewNoteFromSelection()
{
    QString contents;
#ifdef Q_OS_LINUX
    contents = QApplication::clipboard()->text(QClipboard::Selection);
#endif
#ifdef Q_OS_WIN
    int            n = 0;
    QVector<INPUT> input(10);
    memset(input.data(), 0, input.size() * sizeof(INPUT));

    if (GetAsyncKeyState(VK_MENU) & (1 < 15)) {
        input[n].ki.dwFlags = KEYEVENTF_KEYUP;
        input[n].ki.wVk     = VK_MENU;
        input[n++].ki.wScan = MapVirtualKey(VK_MENU, MAPVK_VK_TO_VSC);
    }
    if (GetAsyncKeyState(VK_SHIFT) & (1 < 15)) {
        input[n].ki.dwFlags = KEYEVENTF_KEYUP;
        input[n].ki.wVk     = VK_SHIFT;
        input[n++].ki.wScan = MapVirtualKey(VK_SHIFT, MAPVK_VK_TO_VSC);
    }
    input[n].ki.wVk     = VK_CONTROL;
    input[n].ki.dwFlags = 0;
    input[n++].ki.wScan = MapVirtualKey(VK_CONTROL, MAPVK_VK_TO_VSC);

    input[n].ki.wVk     = 0x43; // Virtual key code for 'c'
    input[n].ki.dwFlags = 0;
    input[n++].ki.wScan = MapVirtualKey(0x56, MAPVK_VK_TO_VSC);

    input[n].ki.dwFlags = KEYEVENTF_KEYUP;
    input[n].ki.wVk     = input[0].ki.wVk;
    input[n++].ki.wScan = input[0].ki.wScan;

    input[n].ki.dwFlags = KEYEVENTF_KEYUP;
    input[n].ki.wVk     = input[1].ki.wVk;
    input[n++].ki.wScan = input[1].ki.wScan;

    bool sent = true;
    for (int i = 0; i < n; i++) {
        input[i].type = INPUT_KEYBOARD;
        if (!SendInput(1, (LPINPUT) & (input[i]), sizeof(INPUT))) {
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
    // copied from
    // http://stackoverflow.com/questions/9758053/programming-with-qt-creator-and-cocoa-copying-the-selected-text-from-the-curre
    CGEventSourceRef source          = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
    CGEventRef       saveCommandDown = CGEventCreateKeyboardEvent(source, (CGKeyCode)8, true);
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
    NoteWidget *nw      = static_cast<NoteWidget *>(sender());
    auto        storage = NoteManager::instance()->storage(nw->storageId());
    if (!nw->noteId().isEmpty()) {
#ifdef MAIN_DEBUG
        qDebug() << "Main::note_trashRequested";
#endif
        storage->deleteNote(nw->noteId());
    }
}

void Main::note_saveRequested()
{
#ifdef MAIN_DEBUG
    qDebug() << "Main::note_saveRequested";
#endif
    NoteWidget *nw        = static_cast<NoteWidget *>(sender());
    QString     storageId = nw->storageId();
    QString     noteId    = nw->noteId();
    QString     text      = nw->text();
    auto        storage   = NoteManager::instance()->storage(storageId);
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
        if (NoteManager::instance()->note(storageId, noteId).text() != text) {
            QString newNoteId = storage->saveNote(noteId, text);
            if (noteId != newNoteId) {
                nw->setNoteId(newNoteId); // this will generate noteIdChanged
            }
        }
    }
}

void Main::note_invalidated()
{
#ifdef MAIN_DEBUG
    qDebug() << "Main::note_invalidated";
#endif
    NoteWidget *nw   = static_cast<NoteWidget *>(sender());
    Note        note = NoteManager::instance()->note(nw->storageId(), nw->noteId());
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
#ifdef MAIN_DEBUG
        qDebug() << "Main::note_removed";
#endif
        dlg->trashRequested();
    }
}

} // namespace QtNote

#include "qtnote.moc"
