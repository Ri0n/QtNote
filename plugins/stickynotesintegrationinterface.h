#ifndef STICKYNOTESINTEGRATIONINTERFACE_H
#define STICKYNOTESINTEGRATIONINTERFACE_H

#include <QRect>
#include <QUuid>

namespace QtNote {

class StickyNotesIntegrationInterface {
public:
    virtual ~StickyNotesIntegrationInterface() = default;

    virtual bool  isStickyNotesAvailable() const                                           = 0;
    virtual bool  presentStickyNote(const QUuid &stickyId, const QRect &preferredGeometry) = 0;
    virtual bool  dismissStickyNote(const QUuid &stickyId)                                 = 0;
    virtual QUuid stickyNoteIdForPresentation(const QString &presentationId) const { return {}; }
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::StickyNotesIntegrationInterface, "com.rion-soft.QtNote.StickyNotesIntegrationInterface/1.0")

#endif // STICKYNOTESINTEGRATIONINTERFACE_H
