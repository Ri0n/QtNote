#include "nextclouddata.h"

#include "nextcloudstorage.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QTimeZone>
#endif

namespace QtNote {

NextcloudData::NextcloudData(NextcloudStorage *storage) : NoteData(storage) { format_ = Note::Markdown; }

QString NextcloudData::storageId() const { return NextcloudStorage::storageId; }

bool NextcloudData::load()
{
    auto *storage = dynamic_cast<NextcloudStorage *>(storage_);
    return storage && storage->loadData(*this);
}

void NextcloudData::remove()
{
    if (storage_ && !id_.isEmpty()) {
        storage_->removeNote(id_);
    }
}

void NextcloudData::initializeNew()
{
    id_.clear();
    etag_.clear();
    title_.clear();
    category_.clear();
    favorite_   = false;
    readOnly_   = false;
    lastChange_ = QDateTime::currentDateTimeUtc();
    setText(QString(), Note::Markdown);
}

void NextcloudData::applyServerNote(const NextcloudRemoteNote &note)
{
    id_       = note.id;
    etag_     = note.etag;
    title_    = note.title;
    category_ = note.category;
    favorite_ = note.favorite;
    readOnly_ = note.readOnly;
    format_   = Note::Markdown;

    if (note.modified > 0) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        lastChange_ = QDateTime::fromSecsSinceEpoch(note.modified, Qt::UTC);
#else
        lastChange_ = QDateTime::fromSecsSinceEpoch(note.modified, QTimeZone(QTimeZone::UTC));
#endif
    } else {
        lastChange_ = {};
    }

    if (note.contentPresent) {
        setText(note.content, Note::Markdown);
    } else {
        text_.clear();
        tags_.clear();
        loaded_ = false;
    }
}

NextcloudRemoteNote NextcloudData::toRemoteNote() const
{
    NextcloudRemoteNote note;
    note.id             = id_;
    note.etag           = etag_;
    note.readOnly       = readOnly_;
    note.content        = text_;
    note.contentPresent = loaded_;
    note.title          = title_;
    note.category       = category_;
    note.favorite       = favorite_;
    note.modified       = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    return note;
}

} // namespace QtNote
