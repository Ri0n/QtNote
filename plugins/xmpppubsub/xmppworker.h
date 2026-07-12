#ifndef XMPPWORKER_H
#define XMPPWORKER_H

#include "xmppdto.h"

#include <QObject>
#include <QSet>
#include <QXmppError.h>

class QXmppClient;
class QXmppDiscoveryManager;
class QXmppPubSubManager;
class QXmppOmemoManager;
class QXmppMessage;
class QJsonObject;

namespace QtNote {

class XmppPepExtension;
class XmppOmemoStorage;

class XmppWorker final : public QObject {
    Q_OBJECT

public:
    explicit XmppWorker(QObject *parent = nullptr);
    ~XmppWorker() override;

    void setConfig(const XmppConfig &config);
    void shutdown();

    XmppStatusResult      probe();
    XmppListResult        listNotes();
    XmppNoteResult        getNote(const QString &id);
    XmppNoteResult        saveNote(const XmppRemoteNote &localNote);
    XmppStatusResult      deleteNote(const QString &id);
    XmppStatusResult      requestStorageKey();
    QList<XmppDeviceInfo> ownOmemoDevices(QString *error);
    XmppStatusResult      trustOwnOmemoDevice(const QByteArray &keyId);

signals:
    void remoteNotePublished(const QtNote::XmppRemoteNote &note);
    void remoteNoteRetracted(const QString &id);
    void remoteNodeInvalidated();
    void connectionChanged(bool connected);
    void workerError(const QString &error);
    void storageKeyReceived(const QByteArray &key);

private:
    void resetClient();
    void createClient();

    XmppStatusResult ensureReady();
    XmppStatusResult ensureOmemo();
    void             handleKeySyncMessage(const QXmppMessage &message);
    void             sendKeySyncMessage(const QString &to, const QJsonObject &payload);
    XmppStatusResult connectToServer();
    XmppStatusResult verifyPrivateStorageSupport();
    XmppStatusResult ensureNode(const QString &nodeName);

    XmppNoteResult requestNote(const QString &id);
    static QString newUuid();
    static QString errorText(const QXmppError &error);

    XmppConfig             config_;
    QXmppClient           *client_ { nullptr };
    QXmppDiscoveryManager *discovery_ { nullptr };
    QXmppPubSubManager    *pubSub_ { nullptr };
    XmppPepExtension      *pepExtension_ { nullptr };
    XmppOmemoStorage      *omemoStorage_ { nullptr };
    QXmppOmemoManager     *omemoManager_ { nullptr };
    bool                   prepared_ { false };
    bool                   omemoReady_ { false };
    QSet<QString>          pendingKeyRequests_;
};

} // namespace QtNote

#endif // XMPPWORKER_H
