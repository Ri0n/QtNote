#ifndef XMPPWORKER_H
#define XMPPWORKER_H

#include "xmppdto.h"

#include <QObject>
#include <QXmppError.h>

class QXmppClient;
class QXmppDiscoveryManager;
class QXmppPubSubManager;
class QXmppRosterManager;
class QXmppOmemoManager;
class QXmppTrustManager;
class QXmppTrustStorage;

namespace QtNote {

class XmppPepExtension;
class XmppOmemoStorage;
class XmppKeySyncExtension;

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
    XmppKeyAuditResult    auditStorageKeys();
    XmppRekeyResult       rekeyStorage(const QList<QByteArray> &keys, const QByteArray &canonicalKey);
    XmppDeviceInfo        ownOmemoDevice() const;
    bool                  ownOmemoBundleValid(QString *error);
    XmppStatusResult      repairOwnOmemoDevice();
    QList<XmppDeviceInfo> ownOmemoDevices(QString *error);
    XmppStatusResult      trustOwnOmemoDevice(const QByteArray &keyId);
    void                  approveKeySyncRequest(const QString &requestId);

signals:
    void remoteNotePublished(const QtNote::XmppRemoteNote &note);
    void remoteNoteRetracted(const QString &id);
    void remoteNodeInvalidated();
    void connectionChanged(bool connected);
    void workerError(const QString &error);
    void keySyncTrustRequested(const QString &requestId, const QByteArray &keyId);

private:
    void             resetClient();
    void             createClient();
    XmppStatusResult ensureReady();
    XmppStatusResult ensureOmemo();
    QStringList      onlineQtNoteResources(QString *error = nullptr);
    void             handleKeySyncRequest(const QString &requestId, const QString &from, const QByteArray &senderKey);
    void handleKeySyncTrustRequest(const QString &requestId, const QString &from, const QByteArray &senderKey);
    void finishKeySyncTrustRequest(const QString &requestId, const QByteArray &senderKey);
    XmppStatusResult connectToServer();
    XmppStatusResult verifyPrivateStorageSupport();
    XmppStatusResult ensureNode(const QString &nodeName);

    XmppNoteResult requestNote(const QString &id);
    static QString newUuid();
    static QString errorText(const QXmppError &error);

    XmppConfig             config_;
    QXmppClient           *client_ { nullptr };
    QXmppDiscoveryManager *discovery_ { nullptr };
    QXmppRosterManager    *roster_ { nullptr };
    QXmppPubSubManager    *pubSub_ { nullptr };
    XmppPepExtension      *pepExtension_ { nullptr };
    XmppKeySyncExtension  *keySyncExtension_ { nullptr };
    XmppOmemoStorage      *omemoStorage_ { nullptr };
    QXmppTrustStorage     *trustStorage_ { nullptr };
    QXmppTrustManager     *trustManager_ { nullptr };
    QXmppOmemoManager     *omemoManager_ { nullptr };
    bool                   prepared_ { false };
    bool                   omemoReady_ { false };
    struct PendingInboundKeyRequest {
        QByteArray senderKey;
        bool       trustBootstrap { false };
    };
    QHash<QString, PendingInboundKeyRequest> pendingInboundKeyRequests_;
};

} // namespace QtNote

#endif // XMPPWORKER_H
