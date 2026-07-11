#include "notestorage.h"

#include <QPointer>
#include <QTimer>

namespace QtNote {

namespace {
    StorageError unavailableError(const QString &message)
    {
        StorageError error;
        error.code      = StorageError::Unavailable;
        error.message   = message;
        error.retryable = true;
        return error;
    }
}

bool NoteStorage::loadNote(Note &target)
{
    if (target.isNull() || target.id().isEmpty())
        return !target.isNull();
    auto loaded = note(target.id());
    if (loaded.isNull())
        return false;
    target = loaded;
    return target.isLoaded();
}

StorageInitJob *NoteStorage::initAsync(QObject *owner)
{
    auto *job = new StorageInitJob(owner ? owner : this);
    job->start();
    QPointer<StorageInitJob> guard(job);
    QTimer::singleShot(0, this, [this, guard]() {
        if (!guard || guard->isFinished())
            return;
        if (init())
            guard->complete();
        else
            guard->fail(unavailableError(tr("Failed to initialize storage %1").arg(name())));
    });
    return job;
}

NoteListJob *NoteStorage::refreshNotesAsync(int limit, QObject *owner)
{
    auto *job = new NoteListJob(owner ? owner : this);
    job->start();
    QPointer<NoteListJob> guard(job);
    QTimer::singleShot(0, this, [this, guard, limit]() {
        if (!guard || guard->isFinished())
            return;
        const auto notes = noteList(limit);
        if (isAccessible())
            guard->complete(notes);
        else
            guard->fail(unavailableError(tr("Storage %1 is unavailable").arg(name())));
    });
    return job;
}

NoteLoadJob *NoteStorage::loadNoteAsync(const QString &id, QObject *owner)
{
    auto *job = new NoteLoadJob(owner ? owner : this);
    job->start();
    QPointer<NoteLoadJob> guard(job);
    QTimer::singleShot(0, this, [this, guard, id]() {
        if (!guard || guard->isFinished())
            return;
        auto loadedNote = note(id);
        if (loadedNote.isNull()) {
            StorageError error;
            error.code    = StorageError::NotFound;
            error.message = tr("Note was not found");
            guard->fail(error);
            return;
        }
        if (!loadedNote.isLoaded() && !loadedNote.load()) {
            guard->fail(unavailableError(tr("Failed to load note from %1").arg(name())));
            return;
        }
        guard->complete(loadedNote);
    });
    return job;
}

NoteSaveJob *NoteStorage::saveNoteAsync(const Note &note, QObject *owner)
{
    auto *job = new NoteSaveJob(owner ? owner : this);
    job->start();
    QPointer<NoteSaveJob> guard(job);
    QTimer::singleShot(0, this, [this, guard, note]() {
        if (!guard || guard->isFinished())
            return;
        if (saveNote(note))
            guard->complete(note);
        else
            guard->fail(unavailableError(tr("Failed to save note to %1").arg(name())));
    });
    return job;
}

NoteRemoveJob *NoteStorage::removeNoteAsync(const QString &id, QObject *owner)
{
    auto *job = new NoteRemoveJob(owner ? owner : this);
    job->start();
    QPointer<NoteRemoveJob> guard(job);
    QTimer::singleShot(0, this, [this, guard, id]() {
        if (!guard || guard->isFinished())
            return;
        removeNote(id);
        guard->complete();
    });
    return job;
}

} // namespace QtNote
