#ifndef CONFLICTRESOLVER_H
#define CONFLICTRESOLVER_H

#include "draftstore.h"
#include "qtnote_export.h"

#include <functional>

namespace QtNote {

/** Context available to a note conflict resolution policy. */
struct QTNOTE_EXPORT NoteConflict {
    DraftRecord localDraft;
    Note        remoteNote;
    QString     message;
};

/** Decision returned by a ConflictResolver. */
struct QTNOTE_EXPORT ConflictResolution {
    enum Action {
        CreateCopy, ///< Publish the local draft as a new note with a new ID.
        KeepDraft,  ///< Preserve the draft for a future/manual decision.
        Discard     ///< Accept the remote version and discard the local draft.
    };

    Action  action { KeepDraft };
    QString copyTitle;
    QString notification;
};

/**
 * @brief Policy interface separating conflict detection from user-visible resolution.
 *
 * Implementations may complete synchronously or asynchronously. The callback
 * must be invoked exactly once while the resolver remains alive.
 */
class QTNOTE_EXPORT ConflictResolver {
public:
    using Completion = std::function<void(ConflictResolution)>;

    virtual ~ConflictResolver()                                        = default;
    virtual void resolve(NoteConflict conflict, Completion completion) = 0;
};

/** Default lossless policy: retain the remote note and publish local text as a copy. */
class QTNOTE_EXPORT CopyConflictResolver final : public ConflictResolver {
public:
    void resolve(NoteConflict conflict, Completion completion) override;
};

} // namespace QtNote

#endif // CONFLICTRESOLVER_H
