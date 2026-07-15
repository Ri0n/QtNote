#ifndef SECUREKEYSTORE_H
#define SECUREKEYSTORE_H

#include "secureenvelope.h"

namespace QtNote {

class QTNOTE_EXPORT SecureKeyStore {
public:
    static CryptoResult<QString> readPassword(const QString &service, const QString &keyName);
    static CryptoError           writePassword(const QString &service, const QString &keyName, const QString &password);
    static CryptoResult<QByteArray> read(const QString &keyName);
    static CryptoError              write(const QString &keyName, const QByteArray &key);
    static CryptoResult<QByteArray> loadOrCreate(const QString &keyName);
};

} // namespace QtNote

#endif // SECUREKEYSTORE_H
