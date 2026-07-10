#ifndef NEXTCLOUDDATA_H
#define NEXTCLOUDDATA_H

#include "nextclouddto.h"
#include "notedata.h"

namespace QtNote {

class NextcloudStorage;

class NextcloudData final : public NoteData {
public:
    explicit NextcloudData(NextcloudStorage *storage);

    QString storageId() const override;
    bool    load() override;
    void    remove() override;

    void                initializeNew();
    void                applyServerNote(const NextcloudRemoteNote &note);
    NextcloudRemoteNote toRemoteNote() const;

    const QString &etag() const { return etag_; }
    const QString &category() const { return category_; }
    bool           favorite() const { return favorite_; }
    bool           readOnly() const { return readOnly_; }

private:
    QString etag_;
    QString category_;
    bool    favorite_ { false };
    bool    readOnly_ { false };
};

} // namespace QtNote

#endif // NEXTCLOUDDATA_H
