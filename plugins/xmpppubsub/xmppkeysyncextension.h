#ifndef XMPPKEYSYNCEXTENSION_H
#define XMPPKEYSYNCEXTENSION_H

#include <QHash>
#include <QXmppClientExtension.h>
#include <QXmppE2eeMetadata.h>
#include <QXmppIq.h>

#include <optional>

namespace QtNote {

class XmppKeySyncExtension final : public QXmppClientExtension {
    Q_OBJECT

public:
    static const QString feature;

    QStringList discoveryFeatures() const override;
    bool        handleStanza(const QDomElement &stanza, const std::optional<QXmppE2eeMetadata> &e2eeMetadata) override;

    QXmppIq makeRequest(const QString &to, const QString &requestId) const;
    void    replyWithKey(const QString &requestId, const QString &recoveryKey);
    void    reject(const QString &requestId);

    static QString responseRecoveryKey(const QDomElement &iq, const QString &requestId);

signals:
    void requestReceived(const QString &requestId, const QString &from, const QByteArray &senderKey);

private:
    struct PendingRequest {
        QString                          from;
        QString                          iqId;
        std::optional<QXmppE2eeMetadata> metadata;
    };

    QHash<QString, PendingRequest> pendingRequests_;
};

} // namespace QtNote

#endif // XMPPKEYSYNCEXTENSION_H
