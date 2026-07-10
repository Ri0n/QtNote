#include "xmppdata.h"

#include "xmppstorage.h"

namespace QtNote {

XmppData::XmppData(XmppStorage *storage) : NoteData(storage) { format_ = Note::Markdown; }

QString XmppData::storageId() const { return XmppStorage::storageId; }

bool XmppData::load()
{
    auto *storage = dynamic_cast<XmppStorage *>(storage_);
    return storage && storage->loadData(*this);
}

void XmppData::remove()
{
    if (storage_ && !id_.isEmpty()) {
        storage_->removeNote(id_);
    }
}

void XmppData::initializeNew()
{
    id_.clear();
    revision_.clear();
    parentRevision_.clear();
    originId_.clear();
    title_.clear();
    lastChange_ = QDateTime::currentDateTimeUtc();
    setText(QString(), Note::Markdown);
}

void XmppData::applyRemoteNote(const XmppRemoteNote &note)
{
    id_             = note.id;
    revision_       = note.revision;
    parentRevision_ = note.parentRevision;
    originId_       = note.originId;
    title_          = note.title;
    lastChange_     = note.modified;
    format_         = Note::Markdown;

    if (note.contentPresent) {
        setText(note.content, Note::Markdown);
    } else {
        text_.clear();
        tags_.clear();
        loaded_ = false;
    }
}

XmppRemoteNote XmppData::toRemoteNote() const
{
    XmppRemoteNote note;
    note.id             = id_;
    note.revision       = revision_;
    note.parentRevision = parentRevision_;
    note.originId       = originId_;
    note.title          = title_;
    note.content        = text_;
    note.modified       = lastChange_;
    note.format         = QStringLiteral("markdown");
    note.contentPresent = loaded_;
    return note;
}

} // namespace QtNote
