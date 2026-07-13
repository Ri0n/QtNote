#ifndef XMPPWORKER_H
#define XMPPWORKER_H

#include "xmppbackend.h"
#include "xmppomemopubsubitems.h"

#include <QCoroTask>
#include <QObject>
#include <QSet>
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

class XmppWorker final : public XmppBackend {
    Q_OBJECT

public:
    explicit XmppWorker(QObject *parent = nullptr);
    ~XmppWorker() override;

    void setConfig(const XmppConfig &config) override;
    void shutdown() override;

    void           probeAsync(StatusCallback callback) override;
    void           listNotesAsync(ListCallback callback) override;
    void           getNoteAsync(const QString &id, NoteCallback callback) override;
    void           saveNoteAsync(const XmppRemoteNote &localNote, NoteCallback callback) override;
    void           deleteNoteAsync(const QString &id, StatusCallback callback) override;
    XmppDeviceInfo ownOmemoDevice() const override;
    void           ownOmemoDevicesAsync(DevicesCallback callback) override;
    void           ownOmemoBundleValidAsync(StatusCallback callback) override;
    void           repairOwnOmemoDeviceAsync(StatusCallback callback) override;
    void           trustOwnOmemoDeviceAsync(const QByteArray &keyId, StatusCallback callback) override;
    void           trustOwnOmemoDevicesAsync(const QList<QByteArray> &keyIds, StatusCallback callback) override;
    void           auditStorageKeysAsync(AuditCallback callback) override;
    void           rekeyStorageAsync(const QList<QByteArray> &keys, const QByteArray &canonicalKey,
                                     RekeyCallback callback) override;
    void           approveKeySyncRequest(const QString &requestId) override;

private:
    void resetClient();
    void createClient();
    void handleKeySyncRequest(const QString &requestId, const QString &from, const QByteArray &senderKey);
    void handleKeySyncTrustRequest(const QString &requestId, const QString &from, const QByteArray &senderKey);
    void finishKeySyncTrustRequest(const QString &requestId, const QByteArray &senderKey);
    void cacheOwnOmemoBundle();
    void scheduleOwnOmemoBundleRepair(uint32_t consumedPreKeyId);
    void repairOwnOmemoBundleAfterPreKeyUse(int attemptsRemaining = 8);
    void connectToServerAsync(StatusCallback callback);
    QCoro::Task<XmppStatusResult> connectToServerTask();
    QCoro::Task<XmppStatusResult> verifyPrivateStorageSupportTask();
    QCoro::Task<XmppStatusResult> verifyNodeTask(const QString &nodeName);
    QCoro::Task<XmppStatusResult> ensureNodeTask(const QString &nodeName);
    QCoro::Task<XmppStatusResult> ensureOmemoTask();
    QCoro::Task<XmppStatusResult> ensureReadyTask();

    QCoro::Task<XmppListResult>                            listNotesTask();
    QCoro::Task<XmppNoteResult>                            requestNoteTask(const QString &id);
    QCoro::Task<XmppNoteResult>                            getNoteTask(const QString &id);
    QCoro::Task<XmppNoteResult>                            publishNoteTask(XmppRemoteNote note);
    QCoro::Task<XmppNoteResult>                            saveNoteTask(XmppRemoteNote localNote);
    QCoro::Task<XmppStatusResult>                          deleteNoteTask(const QString &id);
    QCoro::Task<std::pair<QList<XmppDeviceInfo>, QString>> ownOmemoDevicesTask();
    QCoro::Task<XmppStatusResult>                          ownOmemoBundleValidTask();
    QCoro::Task<XmppStatusResult>                          repairOwnOmemoDeviceTask();
    QCoro::Task<XmppStatusResult>                          trustOwnOmemoDeviceTask(const QByteArray &keyId);
    QCoro::Task<XmppStatusResult>                          trustOwnOmemoDevicesTask(QList<QByteArray> keyIds);
    QCoro::Task<std::pair<QStringList, QString>>           onlineQtNoteResourcesTask();
    QCoro::Task<XmppKeyAuditResult>                        auditStorageKeysTask();
    QCoro::Task<XmppRekeyResult> rekeyStorageTask(QList<QByteArray> keys, QByteArray canonicalKey);
    QCoro::Task<>                approveKeySyncRequestTask(const QString &requestId);
    QCoro::Task<>                handleKeySyncRequestTask(QString requestId, QString from, QByteArray senderKey);
    QCoro::Task<>                cacheOwnOmemoBundleTask();
    QCoro::Task<>                repairOwnOmemoBundleAfterPreKeyUseTask(int attemptsRemaining);
    static QString               newUuid();
    static QString               errorText(const QXmppError &error);

    XmppConfig                         config_;
    QXmppClient                       *client_ { nullptr };
    QXmppDiscoveryManager             *discovery_ { nullptr };
    QXmppRosterManager                *roster_ { nullptr };
    QXmppPubSubManager                *pubSub_ { nullptr };
    XmppPepExtension                  *pepExtension_ { nullptr };
    XmppKeySyncExtension              *keySyncExtension_ { nullptr };
    XmppOmemoStorage                  *omemoStorage_ { nullptr };
    QXmppTrustStorage                 *trustStorage_ { nullptr };
    QXmppTrustManager                 *trustManager_ { nullptr };
    QXmppOmemoManager                 *omemoManager_ { nullptr };
    bool                               prepared_ { false };
    bool                               omemoReady_ { false };
    std::optional<XmppOmemoBundleItem> cachedOwnOmemoBundle_;
    QSet<uint32_t>                     consumedOwnPreKeyIds_;
    bool                               ownBundleRepairScheduled_ { false };
    struct PendingInboundKeyRequest {
        QByteArray senderKey;
        bool       trustBootstrap { false };
    };
    QHash<QString, PendingInboundKeyRequest> pendingInboundKeyRequests_;
};

} // namespace QtNote

#endif // XMPPWORKER_H
