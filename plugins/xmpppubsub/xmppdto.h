#ifndef XMPPPUBSUBDTO_H
#define XMPPPUBSUBDTO_H

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QStringList>

#include <optional>

namespace QtNote {

struct XmppConfig {
    QString    jid;
    QString    password;
    QString    host;
    int        port { 0 };
    QString    resource { QStringLiteral("QtNote") };
    QString    nodeName { QStringLiteral("urn:xmpp:qtnote:notes:0") };
    QString    originId;
    int        timeoutMs { 15000 };
    QByteArray masterKey;
    QByteArray omemoStateKey;
    QString    omemoStatePath;

    QString indexNodeName() const { return nodeName + QStringLiteral(":index:1"); }
    QString contentNodeName() const { return nodeName + QStringLiteral(":content:1"); }
};

struct XmppRemoteNote {
    QString     id;
    QString     revision;
    QString     parentRevision;
    QString     originId;
    QString     title;
    QString     content;
    QDateTime   modified;
    QString     format { QStringLiteral("markdown") };
    QStringList tags;
    bool        contentPresent { true };
};

struct XmppEncryptedPayload {
    enum Kind { Index, Content };

    QString    id;
    Kind       kind { Index };
    quint32    schema { 1 };
    QByteArray keyId;
    QByteArray envelope;
};

struct XmppDeviceInfo {
    QString    label;
    QByteArray keyId;
    int        trustLevel { 0 };
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
Q_DECLARE_METATYPE(QtNote::XmppEncryptedPayload)

#endif // XMPPPUBSUBDTO_H
