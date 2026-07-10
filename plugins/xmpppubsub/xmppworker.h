#ifndef XMPPWORKER_H
#define XMPPWORKER_H

#include "xmppdto.h"

#include <QObject>
#include <QXmppError.h>

class QXmppClient;
class QXmppDiscoveryManager;
class QXmppPubSubManager;

namespace QtNote {

class XmppPepExtension;

class XmppWorker final : public QObject {
    Q_OBJECT

public:
    explicit XmppWorker(QObject *parent = nullptr);
    ~XmppWorker() override;

    void setConfig(const XmppConfig &config);
    void shutdown();

    XmppStatusResult probe();
    XmppListResult   listNotes();
    XmppNoteResult   getNote(const QString &id);
    XmppNoteResult   saveNote(const XmppRemoteNote &localNote);
    XmppStatusResult deleteNote(const QString &id);

signals:
    void remoteNotePublished(const QtNote::XmppRemoteNote &note);
    void remoteNoteRetracted(const QString &id);
    void remoteNodeInvalidated();
    void connectionChanged(bool connected);
    void workerError(const QString &error);

private:
    void resetClient();
    void createClient();

    XmppStatusResult ensureReady();
    XmppStatusResult connectToServer();
    XmppStatusResult verifyPrivateStorageSupport();
    XmppStatusResult ensureNode();

    XmppNoteResult requestNote(const QString &id);
    static QString newUuid();
    static QString errorText(const QXmppError &error);

    XmppConfig             config_;
    QXmppClient           *client_ { nullptr };
    QXmppDiscoveryManager *discovery_ { nullptr };
    QXmppPubSubManager    *pubSub_ { nullptr };
    XmppPepExtension      *pepExtension_ { nullptr };
    bool                   prepared_ { false };
};

} // namespace QtNote

#endif // XMPPWORKER_H
