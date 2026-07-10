#ifndef XMPPDATA_H
#define XMPPDATA_H

#include "notedata.h"
#include "xmppdto.h"

namespace QtNote {

class XmppStorage;

class XmppData final : public NoteData {
public:
    explicit XmppData(XmppStorage *storage);

    QString storageId() const override;
    bool    load() override;
    void    remove() override;

    void           initializeNew();
    void           applyRemoteNote(const XmppRemoteNote &note);
    XmppRemoteNote toRemoteNote() const;

    const QString &revision() const { return revision_; }
    const QString &parentRevision() const { return parentRevision_; }
    const QString &originId() const { return originId_; }

private:
    QString revision_;
    QString parentRevision_;
    QString originId_;
};

} // namespace QtNote

#endif // XMPPDATA_H
