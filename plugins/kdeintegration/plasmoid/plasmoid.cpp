#include "plasmoid.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocale>
#include <QLoggingCategory>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QTranslator>
#include <utility>

#include "qtnote_config.h"

namespace {
constexpr auto ServiceName   = "com.github.ri0n.QtNote";
constexpr auto ObjectPath    = "/QtNote";
constexpr auto InterfaceName = "com.github.ri0n.QtNote";

Q_LOGGING_CATEGORY(logPlasmoid, "qtnote.plasmoid")

void loadTranslations()
{
    static bool loaded = false;
    if (std::exchange(loaded, true))
        return;

    QSettings     settings(QStringLiteral("R-Soft"), QStringLiteral("QtNote"));
    const QString forcedLangName = settings.value(QLatin1String("language")).toString();
    const bool    autoLang       = forcedLangName.isEmpty() || forcedLangName == QLatin1String("auto");
    const QLocale locale         = autoLang ? QLocale::system() : QLocale(forcedLangName);

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QLatin1String("/R-Soft/QtNote/translations");
    QStringList langDirs {
        QStringLiteral(TRANSLATIONSDIR),
        dataDir,
    };
#ifdef QTNOTE_DEVEL_TRANSLATIONSDIR
    langDirs.prepend(QStringLiteral(QTNOTE_DEVEL_TRANSLATIONSDIR));
#endif

    auto *translator = new QTranslator(qApp);
    for (const QString &langDir : langDirs) {
        if (translator->load(locale, QStringLiteral(APPNAME), QStringLiteral("_"), langDir)) {
            qApp->installTranslator(translator);
            return;
        }
    }

    delete translator;
}
}

NotesModel::NotesModel(QObject *parent) : QAbstractListModel(parent)
{
    loadTranslations();

    auto bus         = QDBusConnection::sessionBus();
    m_serviceWatcher = new QDBusServiceWatcher(
        QLatin1String(ServiceName), bus,
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration, this);

    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &NotesModel::serviceRegistered);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &NotesModel::serviceUnregistered);
    bus.connect(QLatin1String(ServiceName), QLatin1String(ObjectPath), QLatin1String(InterfaceName),
                QStringLiteral("notesChanged"), this, SLOT(refresh()));

    const auto reply = bus.interface()->isServiceRegistered(QLatin1String(ServiceName));
    setAvailable(reply.isValid() && reply.value());
    if (m_available) {
        createInterface();
        refresh();
    }
}

int NotesModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : m_items.size(); }

QVariant NotesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};

    const auto &item = m_items.at(index.row());
    switch (role) {
    case NoteIdRole:
        return item.id;
    case StorageIdRole:
        return item.storageId;
    case TitleRole:
        return item.title;
    case ModifiedRole:
        return item.modified;
    default:
        return {};
    }
}

QHash<int, QByteArray> NotesModel::roleNames() const
{
    return {
        { NoteIdRole, "noteId" },
        { StorageIdRole, "storageId" },
        { TitleRole, "title" },
        { ModifiedRole, "modified" },
    };
}

bool NotesModel::available() const { return m_available; }
bool NotesModel::loading() const { return m_loading; }

void NotesModel::refresh()
{
    if (!m_available || !m_interface) {
        startBackend();
        return;
    }

    setLoading(true);
    const quint64 requestSerial = ++m_requestSerial;
    auto         *watcher = new QDBusPendingCallWatcher(m_interface->asyncCall(QStringLiteral("notesJson")), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, requestSerial]() {
        const QDBusPendingReply<QString> reply = *watcher;
        watcher->deleteLater();
        if (requestSerial != m_requestSerial)
            return;

        setLoading(false);
        if (reply.isError()) {
            qCWarning(logPlasmoid) << "Failed to fetch notes:" << reply.error().message();
            return;
        }

        QJsonParseError error;
        const auto      document = QJsonDocument::fromJson(reply.value().toUtf8(), &error);
        if (error.error != QJsonParseError::NoError || !document.isArray()) {
            qCWarning(logPlasmoid) << "Invalid notes response:" << error.errorString();
            return;
        }

        QList<Item> items;
        const auto  array = document.array();
        items.reserve(array.size());
        for (const auto &value : array) {
            const auto object = value.toObject();
            Item       item {
                object.value(QStringLiteral("id")).toString(),
                object.value(QStringLiteral("storageId")).toString(),
                object.value(QStringLiteral("title")).toString(),
                object.value(QStringLiteral("modified")).toString(),
            };
            if (!item.id.isEmpty() && !item.storageId.isEmpty())
                items.append(std::move(item));
        }

        beginResetModel();
        m_items = std::move(items);
        endResetModel();
        emit countChanged();
    });
}

void NotesModel::openNote(int row)
{
    if (!m_available || !m_interface || row < 0 || row >= m_items.size())
        return;
    const auto &item = m_items.at(row);
    m_interface->asyncCall(QStringLiteral("openNote"), item.storageId, item.id);
}

void NotesModel::createNote() { callOrStart(QStringLiteral("createNote")); }
void NotesModel::showNoteManager() { callOrStart(QStringLiteral("showNoteManager")); }
void NotesModel::showOptions() { callOrStart(QStringLiteral("showOptions")); }
void NotesModel::showAbout() { callOrStart(QStringLiteral("showAbout")); }
void NotesModel::quit() { call(QStringLiteral("quit")); }

void NotesModel::serviceRegistered()
{
    createInterface();
    m_starting = false;
    setAvailable(true);
    refresh();
    runPendingCall();
}

void NotesModel::serviceUnregistered()
{
    ++m_requestSerial;
    setLoading(false);
    setAvailable(false);
    delete m_interface;
    m_interface = nullptr;
    if (!m_items.isEmpty()) {
        beginResetModel();
        m_items.clear();
        endResetModel();
        emit countChanged();
    }
}

void NotesModel::setAvailable(bool available)
{
    if (m_available == available)
        return;
    m_available = available;
    emit availableChanged();
}

void NotesModel::setLoading(bool loading)
{
    if (m_loading == loading)
        return;
    m_loading = loading;
    emit loadingChanged();
}

void NotesModel::call(const QString &method)
{
    if (m_available && m_interface)
        m_interface->asyncCall(method);
}

bool NotesModel::startBackend()
{
    if (m_starting)
        return true;

#ifdef QTNOTE_DEVEL_EXECUTABLE
    const QString executable = QStringLiteral(QTNOTE_DEVEL_EXECUTABLE);
#else
    const QString executable = QStandardPaths::findExecutable(QStringLiteral("qtnote"));
#endif
    if (executable.isEmpty()) {
        qCWarning(logPlasmoid) << "Unable to find qtnote executable";
        return false;
    }

    if (!QProcess::startDetached(executable)) {
        qCWarning(logPlasmoid) << "Failed to start qtnote executable:" << executable;
        return false;
    }

    m_starting = true;
    setLoading(true);
    QTimer::singleShot(10000, this, [this]() {
        if (m_available)
            return;

        m_starting = false;
        m_pendingCall.clear();
        setLoading(false);
    });
    return true;
}

void NotesModel::callOrStart(const QString &method)
{
    if (m_available && m_interface) {
        m_interface->asyncCall(method);
        return;
    }

    m_pendingCall = method;
    if (!startBackend())
        m_pendingCall.clear();
}

void NotesModel::runPendingCall()
{
    if (m_pendingCall.isEmpty() || !m_available || !m_interface)
        return;

    const QString method = std::exchange(m_pendingCall, QString());
    m_interface->asyncCall(method);
}

void NotesModel::createInterface()
{
    delete m_interface;
    m_interface = new QDBusInterface(QLatin1String(ServiceName), QLatin1String(ObjectPath),
                                     QLatin1String(InterfaceName), QDBusConnection::sessionBus(), this);
}
