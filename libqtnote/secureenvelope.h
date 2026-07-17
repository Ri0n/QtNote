#ifndef SECUREENVELOPE_H
#define SECUREENVELOPE_H

#include "qtnote_export.h"

#include <QByteArray>
#include <QString>

namespace QtNote {

enum class KeyDomain : quint8 {
    LocalDraft = 1,
    StorageIndex,
    StorageContent,
    StorageKeyTransport,
    OmemoState,
    LocalRemoteCache
};

struct QTNOTE_EXPORT AeadContext {
    KeyDomain domain { KeyDomain::LocalDraft };
    QString   container;
    QString   itemId;
    quint32   schema { 1 };
    QString   kind;
};

struct QTNOTE_EXPORT CryptoError {
    enum Code { None, InvalidArgument, Unavailable, AuthenticationFailed, Corrupt };

    Code    code { None };
    QString message;

    explicit operator bool() const { return code != None; }
};

template <typename T> struct CryptoResult {
    T           value;
    CryptoError error;

    explicit operator bool() const { return !error; }
};

class QTNOTE_EXPORT SecureEnvelope {
public:
    static constexpr int MasterKeySize = 32;

    static bool                     isAvailable();
    static QByteArray               generateMasterKey();
    static QByteArray               keyId(const QByteArray &masterKey);
    static QString                  encodeRecoveryKey(const QByteArray &masterKey);
    static CryptoResult<QByteArray> decodeRecoveryKey(const QString &encoded);
    static QByteArray               deriveKey(const QByteArray &masterKey, KeyDomain domain);
    static QByteArray               associatedData(const AeadContext &context);

    static CryptoResult<QByteArray> seal(const QByteArray &plainText, const QByteArray &masterKey,
                                         const AeadContext &context);
    static CryptoResult<QByteArray> open(const QByteArray &envelope, const QByteArray &masterKey,
                                         const AeadContext &context);
};

} // namespace QtNote

#endif // SECUREENVELOPE_H
