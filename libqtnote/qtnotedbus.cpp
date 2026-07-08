/*
    SPDX-License-Identifier: GPL-3.0-only
*/

#include "qtnotedbus.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

#include "notemanager.h"
#include "qtnote.h"

namespace QtNote {

namespace {
    constexpr auto ServiceName          = "com.github.ri0n.QtNote";
    constexpr auto ObjectPath           = "/QtNote";
    constexpr int  DefaultNotesPageSize = 50;
    constexpr int  MaxNotesPageSize     = 200;
}

QtNoteDBus::QtNoteDBus(Main *qtnote, QObject *parent) : QObject(parent)
{
    auto *manager = NoteManager::instance();
    connect(manager, &NoteManager::storageAdded, this, &QtNoteDBus::notesChanged);
    connect(manager, &NoteManager::storageRemoved, this, &QtNoteDBus::notesChanged);
    connect(manager, qOverload<NoteStorage::Ptr>(&NoteManager::storageChanged), this, &QtNoteDBus::notesChanged);
    connect(qtnote, &Main::settingsUpdated, this, &QtNoteDBus::notesChanged);

    auto bus = QDBusConnection::sessionBus();
    if (!bus.registerObject(QLatin1String(ObjectPath), this,
                            QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals)) {
        qWarning("Failed to register the QtNote D-Bus object: %s", qPrintable(bus.lastError().message()));
        return;
    }

    m_registeredService = bus.registerService(QLatin1String(ServiceName));
    if (!m_registeredService) {
        qWarning("Failed to register the QtNote D-Bus service: %s", qPrintable(bus.lastError().message()));
        bus.unregisterObject(QLatin1String(ObjectPath));
    }
}

QtNoteDBus::~QtNoteDBus()
{
    if (!m_registeredService)
        return;

    auto bus = QDBusConnection::sessionBus();
    bus.unregisterObject(QLatin1String(ObjectPath));
    bus.unregisterService(QLatin1String(ServiceName));
}

QString QtNoteDBus::notesJson(int offset, int limit, const QString &query) const
{
    offset = qMax(0, offset);
    limit  = limit > 0 ? qMin(limit, MaxNotesPageSize) : DefaultNotesPageSize;

    const auto notes = NoteManager::instance()->noteList(offset, limit + 1, query);

    QJsonArray result;
    for (int i = 0, count = qMin(limit, notes.size()); i < count; ++i) {
        const auto &note = notes.at(i);
        result.append(QJsonObject {
            { "id", note.id },
            { "storageId", note.storageId },
            { "title", note.title },
            { "modified", note.lastModify.toUTC().toString(Qt::ISODateWithMs) },
        });
    }
    return QString::fromUtf8(QJsonDocument(QJsonObject {
                                               { "notes", result },
                                               { "hasMore", notes.size() > limit },
                                           })
                                 .toJson(QJsonDocument::Compact));
}

void QtNoteDBus::openNote(const QString &storageId, const QString &noteId)
{
    if (!storageId.isEmpty() && !noteId.isEmpty())
        emit openNoteRequested(storageId, noteId);
}

void QtNoteDBus::createNote() { emit createNoteRequested(); }
void QtNoteDBus::showNoteManager() { emit noteManagerRequested(); }
void QtNoteDBus::showOptions() { emit optionsRequested(); }
void QtNoteDBus::showAbout() { emit aboutRequested(); }
void QtNoteDBus::quit() { emit quitRequested(); }

} // namespace QtNote
