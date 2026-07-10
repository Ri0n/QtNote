#ifndef XMPPPUBSUBDTO_H
#define XMPPPUBSUBDTO_H

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QString>

#include <optional>

namespace QtNote {

struct XmppConfig {
    QString jid;
    QString password;
    QString host;
    int     port { 0 };
    QString resource { QStringLiteral("QtNote") };
    QString nodeName { QStringLiteral("urn:xmpp:qtnote:notes:0") };
    QString originId;
    int     timeoutMs { 15000 };
};

struct XmppRemoteNote {
    QString   id;
    QString   revision;
    QString   parentRevision;
    QString   originId;
    QString   title;
    QString   content;
    QDateTime modified;
    QString   format { QStringLiteral("markdown") };
    bool      contentPresent { true };
};

struct XmppStatusResult {
    bool                          ok { false };
    bool                          conflict { false };
    bool                          notFound { false };
    QString                       error;
    std::optional<XmppRemoteNote> remoteOnConflict;
};

struct XmppListResult : XmppStatusResult {
    QList<XmppRemoteNote> notes;
    bool                  partial { false };
};

struct XmppNoteResult : XmppStatusResult {
    XmppRemoteNote note;
};

} // namespace QtNote

Q_DECLARE_METATYPE(QtNote::XmppRemoteNote)

#endif // XMPPPUBSUBDTO_H
