#ifndef XMPPBACKEND_H
#define XMPPBACKEND_H

#include "xmppdto.h"

#include <QObject>
#include <functional>

namespace QtNote {

class XmppBackend : public QObject {
    Q_OBJECT

public:
    using StatusCallback  = std::function<void(XmppStatusResult)>;
    using ListCallback    = std::function<void(XmppListResult)>;
    using NoteCallback    = std::function<void(XmppNoteResult)>;
    using DevicesCallback = std::function<void(QList<XmppDeviceInfo>, QString)>;
    using AuditCallback   = std::function<void(XmppKeyAuditResult)>;
    using RekeyCallback   = std::function<void(XmppRekeyResult)>;

    using QObject::QObject;
    ~XmppBackend() override = default;

    virtual void           setConfig(const XmppConfig &config)                                                 = 0;
    virtual void           shutdown()                                                                          = 0;
    virtual void           probeAsync(StatusCallback callback)                                                 = 0;
    virtual void           listNotesAsync(ListCallback callback)                                               = 0;
    virtual void           getNoteAsync(const QString &id, NoteCallback callback)                              = 0;
    virtual void           saveNoteAsync(const XmppRemoteNote &note, NoteCallback callback)                    = 0;
    virtual void           deleteNoteAsync(const QString &id, StatusCallback callback)                         = 0;
    virtual XmppDeviceInfo ownOmemoDevice() const                                                              = 0;
    virtual void           ownOmemoDevicesAsync(DevicesCallback callback)                                      = 0;
    virtual void           ownOmemoBundleValidAsync(StatusCallback callback)                                   = 0;
    virtual void           repairOwnOmemoDeviceAsync(StatusCallback callback)                                  = 0;
    virtual void           trustOwnOmemoDeviceAsync(const QByteArray &keyId, StatusCallback callback)          = 0;
    virtual void           trustOwnOmemoDevicesAsync(const QList<QByteArray> &keyIds, StatusCallback callback) = 0;
    virtual void           auditStorageKeysAsync(AuditCallback callback)                                       = 0;
    virtual void           rekeyStorageAsync(const QList<QByteArray> &keys, const QByteArray &canonicalKey,
                                             RekeyCallback callback)
        = 0;
    virtual void approveKeySyncRequest(const QString &requestId) = 0;

signals:
    void remoteNotePublished(const QtNote::XmppRemoteNote &note);
    void remoteNoteRetracted(const QString &id);
    void remoteNodeInvalidated();
    void connectionChanged(bool connected);
    void backendError(const QString &error);
    void keySyncTrustRequested(const QString &requestId, const QByteArray &keyId);
};

} // namespace QtNote

#endif // XMPPBACKEND_H
