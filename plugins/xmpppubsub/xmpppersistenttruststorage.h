#ifndef XMPPPERSISTENTTRUSTSTORAGE_H
#define XMPPPERSISTENTTRUSTSTORAGE_H

#include <QList>
#include <QXmppTrustMemoryStorage.h>

namespace QtNote {

class XmppPersistentTrustStorage final : public QXmppTrustMemoryStorage {
public:
    XmppPersistentTrustStorage(QString path, QByteArray encryptionKey, QString accountId);

    bool    isValid() const { return error_.isEmpty(); }
    QString errorString() const { return error_; }

    QXmppTask<QHash<QString, QMultiHash<QString, QByteArray>>>
    setTrustLevel(const QString &encryption, const QMultiHash<QString, QByteArray> &keyIds,
                  QXmpp::TrustLevel trustLevel) override;
    QXmppTask<QHash<QString, QMultiHash<QString, QByteArray>>> setTrustLevel(const QString        &encryption,
                                                                             const QList<QString> &keyOwnerJids,
                                                                             QXmpp::TrustLevel     oldTrustLevel,
                                                                             QXmpp::TrustLevel newTrustLevel) override;
    QXmppTask<void> removeKeys(const QString &encryption, const QList<QByteArray> &keyIds) override;
    QXmppTask<void> removeKeys(const QString &encryption, const QString &keyOwnerJid) override;
    QXmppTask<void> removeKeys(const QString &encryption) override;
    QXmppTask<void> resetAll(const QString &encryption) override;

private:
    struct Entry {
        QString           encryption;
        QString           owner;
        QByteArray        keyId;
        QXmpp::TrustLevel level;
    };

    void load();
    void persist();

    QString      path_;
    QByteArray   encryptionKey_;
    QString      accountId_;
    QList<Entry> entries_;
    QString      error_;
};

} // namespace QtNote

#endif // XMPPPERSISTENTTRUSTSTORAGE_H
