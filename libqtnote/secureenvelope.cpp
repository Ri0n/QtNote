#include "secureenvelope.h"

#include <QtCrypto>

#include <QCryptographicHash>
#include <QDataStream>

namespace QtNote {
namespace {
    constexpr quint32 EnvelopeMagic   = 0x514e5345; // QNSE
    constexpr quint16 EnvelopeVersion = 1;
    constexpr quint32 AadMagic        = 0x514e4144; // QNAD
    constexpr quint16 AadVersion      = 1;
    constexpr int     NonceSize       = 12;
    constexpr int     TagSize         = 16;

    QCA::Initializer &qcaInitializer()
    {
        static QCA::Initializer initializer;
        return initializer;
    }

    CryptoError error(CryptoError::Code code, const QString &message) { return { code, message }; }

    QByteArray hmacSha256(QByteArray key, const QByteArray &message)
    {
        constexpr int BlockSize = 64;
        if (key.size() > BlockSize)
            key = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
        key.resize(BlockSize, '\0');
        QByteArray outer(BlockSize, char(0x5c));
        QByteArray inner(BlockSize, char(0x36));
        for (int i = 0; i < BlockSize; ++i) {
            outer[i] = char(uchar(outer[i]) ^ uchar(key[i]));
            inner[i] = char(uchar(inner[i]) ^ uchar(key[i]));
        }
        const auto innerHash = QCryptographicHash::hash(inner + message, QCryptographicHash::Sha256);
        return QCryptographicHash::hash(outer + innerHash, QCryptographicHash::Sha256);
    }

    QByteArray domainLabel(KeyDomain domain)
    {
        switch (domain) {
        case KeyDomain::LocalDraft:
            return QByteArrayLiteral("local-draft");
        case KeyDomain::StorageIndex:
            return QByteArrayLiteral("storage-index");
        case KeyDomain::StorageContent:
            return QByteArrayLiteral("storage-content");
        case KeyDomain::StorageKeyTransport:
            return QByteArrayLiteral("storage-key-transport");
        case KeyDomain::OmemoState:
            return QByteArrayLiteral("omemo-state");
        }
        return {};
    }

} // namespace

bool SecureEnvelope::isAvailable()
{
    qcaInitializer();
    return QCA::haveSecureRandom() && QCA::isSupported("aes256-gcm");
}

QByteArray SecureEnvelope::generateMasterKey()
{
    qcaInitializer();
    return QCA::haveSecureRandom() ? QCA::Random::randomArray(MasterKeySize).toByteArray() : QByteArray {};
}

QByteArray SecureEnvelope::keyId(const QByteArray &masterKey)
{
    if (masterKey.size() != MasterKeySize)
        return {};
    return QCryptographicHash::hash(QByteArrayLiteral("QtNote storage key id v1\0") + masterKey,
                                    QCryptographicHash::Sha256);
}

QString SecureEnvelope::encodeRecoveryKey(const QByteArray &masterKey)
{
    const auto id = keyId(masterKey);
    if (id.isEmpty())
        return {};
    return QStringLiteral("qtnote-key-v1:%1:%2")
        .arg(QString::fromLatin1(masterKey.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)),
             QString::fromLatin1(id.left(6).toHex()));
}

CryptoResult<QByteArray> SecureEnvelope::decodeRecoveryKey(const QString &encoded)
{
    const auto parts = encoded.trimmed().split(QLatin1Char(':'));
    if (parts.size() != 3 || parts.at(0) != QStringLiteral("qtnote-key-v1"))
        return { {}, error(CryptoError::InvalidArgument, QStringLiteral("Invalid recovery key format")) };
    const auto key = QByteArray::fromBase64(parts.at(1).toLatin1(), QByteArray::Base64UrlEncoding);
    if (key.size() != MasterKeySize || QString::fromLatin1(keyId(key).left(6).toHex()) != parts.at(2).toLower())
        return { {}, error(CryptoError::AuthenticationFailed, QStringLiteral("Invalid recovery key checksum")) };
    return { key, {} };
}

QByteArray SecureEnvelope::deriveKey(const QByteArray &masterKey, KeyDomain domain)
{
    if (masterKey.size() != MasterKeySize)
        return {};
    // RFC 5869 extract+expand with fixed protocol salt and a one-block output.
    const auto prk = hmacSha256(QByteArrayLiteral("QtNote HKDF salt v1"), masterKey);
    return hmacSha256(prk, QByteArrayLiteral("QtNote key domain v1:") + domainLabel(domain) + QByteArray(1, '\1'));
}

QByteArray SecureEnvelope::associatedData(const AeadContext &context)
{
    QByteArray  result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_10);
    stream << AadMagic << AadVersion << quint8(context.domain) << context.container << context.itemId << context.schema
           << context.kind;
    return result;
}

CryptoResult<QByteArray> SecureEnvelope::seal(const QByteArray &plainText, const QByteArray &masterKey,
                                              const AeadContext &context)
{
    if (masterKey.size() != MasterKeySize || context.container.isEmpty() || context.itemId.isEmpty()
        || context.kind.isEmpty()) {
        return { {}, error(CryptoError::InvalidArgument, QStringLiteral("Invalid encryption key or context")) };
    }
    if (!isAvailable())
        return { {}, error(CryptoError::Unavailable, QStringLiteral("AES-256-GCM is unavailable")) };
    const auto  key   = deriveKey(masterKey, context.domain);
    const auto  nonce = QCA::Random::randomArray(NonceSize).toByteArray();
    QByteArray  authenticatedPlainText;
    QDataStream framed(&authenticatedPlainText, QIODevice::WriteOnly);
    framed.setVersion(QDataStream::Qt_5_10);
    framed << associatedData(context) << plainText;
    QCA::Cipher cipher(QStringLiteral("aes256"), QCA::Cipher::GCM, QCA::Cipher::NoPadding, QCA::Encode,
                       QCA::SymmetricKey(key), QCA::InitializationVector(nonce), QCA::AuthTag(TagSize));
    const auto  cipherText = cipher.process(QCA::SecureArray(authenticatedPlainText)).toByteArray();
    if (!cipher.ok() || cipher.tag().size() != TagSize)
        return { {}, error(CryptoError::Unavailable, QStringLiteral("Encryption failed")) };

    QByteArray  envelope;
    QDataStream out(&envelope, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);
    out << EnvelopeMagic << EnvelopeVersion << nonce << cipher.tag().toByteArray() << cipherText;
    return { envelope, {} };
}

CryptoResult<QByteArray> SecureEnvelope::open(const QByteArray &envelope, const QByteArray &masterKey,
                                              const AeadContext &context)
{
    if (masterKey.size() != MasterKeySize)
        return { {}, error(CryptoError::InvalidArgument, QStringLiteral("Invalid encryption key")) };
    if (!isAvailable())
        return { {}, error(CryptoError::Unavailable, QStringLiteral("AES-256-GCM is unavailable")) };
    quint32     magic;
    quint16     version;
    QByteArray  nonce;
    QByteArray  tag;
    QByteArray  cipherText;
    QDataStream in(envelope);
    in.setVersion(QDataStream::Qt_5_10);
    in >> magic >> version >> nonce >> tag >> cipherText;
    if (in.status() != QDataStream::Ok || magic != EnvelopeMagic || version != EnvelopeVersion
        || nonce.size() != NonceSize || tag.size() != TagSize) {
        return { {}, error(CryptoError::Corrupt, QStringLiteral("Invalid encrypted envelope")) };
    }
    const auto  key = deriveKey(masterKey, context.domain);
    QCA::Cipher cipher(QStringLiteral("aes256"), QCA::Cipher::GCM, QCA::Cipher::NoPadding, QCA::Decode,
                       QCA::SymmetricKey(key), QCA::InitializationVector(nonce), QCA::AuthTag(tag));
    const auto  authenticatedPlainText = cipher.process(QCA::SecureArray(cipherText)).toByteArray();
    if (!cipher.ok())
        return { {}, error(CryptoError::AuthenticationFailed, QStringLiteral("Authentication failed")) };
    QByteArray  embeddedContext;
    QByteArray  plainText;
    QDataStream framed(authenticatedPlainText);
    framed.setVersion(QDataStream::Qt_5_10);
    framed >> embeddedContext >> plainText;
    if (framed.status() != QDataStream::Ok || embeddedContext != associatedData(context))
        return { {}, error(CryptoError::AuthenticationFailed, QStringLiteral("Encryption context mismatch")) };
    return { plainText, {} };
}

} // namespace QtNote
