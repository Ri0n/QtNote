#ifndef STICKYNOTESMANAGER_H
#define STICKYNOTESMANAGER_H

#include <QHash>
#include <QObject>
#include <QRect>
#include <QUuid>

#include "note.h"

namespace QtNote {

class Main;
class StickyNotesIntegrationInterface;

class StickyNotesManager : public QObject {
    Q_OBJECT
public:
    explicit StickyNotesManager(Main *main);

    void setBackend(StickyNotesIntegrationInterface *backend);
    bool isAvailable() const;

    void requestPin(const Note &note, const QUuid &draftId, bool awaitingPublication, const QRect &preferredGeometry);
    bool unpin(const QUuid &stickyId);
    bool open(const QUuid &stickyId);

    QString notesJson() const;
    QString noteJson(const QUuid &stickyId) const;
    QString noteForPresentationJson(const QString &presentationId) const;

signals:
    void availabilityChanged(bool available);
    void notesChanged();

private:
    struct Record {
        QUuid   id;
        QString storageId;
        QString noteId;
    };
    struct PendingPin {
        QRect preferredGeometry;
    };

    void          load();
    void          save(const Record &record);
    void          remove(const QUuid &stickyId);
    void          watchStorage(NoteStorage *storage);
    QUuid         pinPublished(const Note &note, const QRect &preferredGeometry);
    const Record *record(const QUuid &stickyId) const;

    Main                            *main_    = nullptr;
    StickyNotesIntegrationInterface *backend_ = nullptr;
    QHash<QUuid, Record>             records_;
    QHash<QUuid, PendingPin>         pendingPins_;
};

} // namespace QtNote

#endif // STICKYNOTESMANAGER_H
