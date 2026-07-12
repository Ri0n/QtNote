#include "filedraftstore.h"

#include <QtCrypto>

#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace QtNote {
namespace {
    constexpr quint32 EnvelopeMagic   = 0x514e4445; // QNDE
    constexpr quint32 PayloadMagic    = 0x514e4450; // QNDP
    constexpr quint16 EnvelopeVersion = 1;
    constexpr quint16 PayloadVersion  = 1;
    constexpr int     NonceSize       = 12;
    constexpr int     TagSize         = 16;

    QCA::Initializer &qcaInitializer()
    {
        static QCA::Initializer initializer;
        return initializer;
    }

    DraftStoreError error(DraftStoreError::Code code, const QString &message) { return { code, message }; }

    QByteArray serialize(const DraftRecord &record)
    {
        QByteArray  bytes;
        QDataStream out(&bytes, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_10);
        out << PayloadMagic << PayloadVersion << record.id << quint8(record.state) << record.storageId
            << record.remoteNoteId << record.title << record.body << quint8(record.format) << record.tags
            << record.updatedAt << record.lastError << record.retryAt;
        return bytes;
    }

    DraftStoreResult<DraftRecord> deserialize(const QByteArray &bytes)
    {
        DraftRecord record;
        quint32     magic;
        quint16     version;
        quint8      state;
        quint8      format;
        QDataStream in(bytes);
        in.setVersion(QDataStream::Qt_5_10);
        in >> magic >> version;
        if (magic != PayloadMagic || version != PayloadVersion)
            return { {}, error(DraftStoreError::Corrupt, QStringLiteral("Unsupported draft payload")) };
        in >> record.id >> state >> record.storageId >> record.remoteNoteId >> record.title >> record.body >> format
            >> record.tags >> record.updatedAt >> record.lastError >> record.retryAt;
        if (in.status() != QDataStream::Ok || record.id.isNull() || state > DraftRecord::NeedsRouting
            || format > Note::Html) {
            return { {}, error(DraftStoreError::Corrupt, QStringLiteral("Invalid draft payload")) };
        }
        record.state  = DraftRecord::State(state);
        record.format = Note::Format(format);
        return { record, {} };
    }
} // namespace

FileDraftStore::FileDraftStore(QString rootPath, QByteArray masterKey) :
    rootPath_(std::move(rootPath)), masterKey_(std::move(masterKey))
{
    qcaInitializer();
}

QByteArray FileDraftStore::generateMasterKey()
{
    qcaInitializer();
    if (!QCA::haveSecureRandom())
        return {};
    return QCA::Random::randomArray(MasterKeySize).toByteArray();
}

bool FileDraftStore::cryptoAvailable()
{
    qcaInitializer();
    return QCA::haveSecureRandom() && QCA::isSupported("aes256-gcm");
}

QString FileDraftStore::pathFor(const QUuid &id, DraftRecord::State) const
{
    return QDir(rootPath_).filePath(id.toString(QUuid::WithoutBraces) + QStringLiteral(".draft"));
}

QString FileDraftStore::findPath(const QUuid &id, DraftRecord::State *state) const
{
    if (state)
        *state = DraftRecord::Editing;
    const auto path = pathFor(id, DraftRecord::Editing);
    return QFileInfo::exists(path) ? path : QString();
}

DraftStoreError FileDraftStore::ensureDirectories() const
{
    if (masterKey_.size() != MasterKeySize)
        return error(DraftStoreError::Locked, QStringLiteral("Draft store master key is unavailable"));
    if (!cryptoAvailable())
        return error(DraftStoreError::CryptoUnavailable, QStringLiteral("AES-256-GCM is unavailable"));
    QDir dir;
    if (!dir.mkpath(rootPath_))
        return error(DraftStoreError::Io, QStringLiteral("Failed to create draft directory"));
    QFile::setPermissions(rootPath_, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    return {};
}

DraftStoreResult<QByteArray> FileDraftStore::encrypt(const QByteArray &plainText) const
{
    if (auto e = ensureDirectories())
        return { {}, e };
    const auto  nonce = QCA::Random::randomArray(NonceSize).toByteArray();
    QCA::Cipher cipher(QStringLiteral("aes256"), QCA::Cipher::GCM, QCA::Cipher::NoPadding, QCA::Encode,
                       QCA::SymmetricKey(masterKey_), QCA::InitializationVector(nonce), QCA::AuthTag(TagSize));
    const auto  cipherText = cipher.process(QCA::SecureArray(plainText)).toByteArray();
    if (!cipher.ok() || cipher.tag().size() != TagSize)
        return { {}, error(DraftStoreError::CryptoUnavailable, QStringLiteral("Draft encryption failed")) };

    QByteArray  envelope;
    QDataStream out(&envelope, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);
    out << EnvelopeMagic << EnvelopeVersion << nonce << cipher.tag().toByteArray() << cipherText;
    return { envelope, {} };
}

DraftStoreResult<QByteArray> FileDraftStore::decrypt(const QByteArray &envelope) const
{
    if (masterKey_.size() != MasterKeySize)
        return { {}, error(DraftStoreError::Locked, QStringLiteral("Draft store master key is unavailable")) };
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
        return { {}, error(DraftStoreError::Corrupt, QStringLiteral("Invalid draft envelope")) };
    }
    QCA::Cipher cipher(QStringLiteral("aes256"), QCA::Cipher::GCM, QCA::Cipher::NoPadding, QCA::Decode,
                       QCA::SymmetricKey(masterKey_), QCA::InitializationVector(nonce), QCA::AuthTag(tag));
    const auto  plainText = cipher.process(QCA::SecureArray(cipherText)).toByteArray();
    if (!cipher.ok())
        return { {}, error(DraftStoreError::Corrupt, QStringLiteral("Draft authentication failed")) };
    return { plainText, {} };
}

DraftStoreError FileDraftStore::write(const DraftRecord &source)
{
    if (source.id.isNull())
        return error(DraftStoreError::InvalidArgument, QStringLiteral("Draft id is empty"));
    auto record = source;
    if (!record.updatedAt.isValid())
        record.updatedAt = QDateTime::currentDateTimeUtc();
    auto encrypted = encrypt(serialize(record));
    if (!encrypted)
        return encrypted.error;
    QSaveFile file(pathFor(record.id, record.state));
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly))
        return error(DraftStoreError::Io, file.errorString());
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    if (file.write(encrypted.value) != encrypted.value.size() || !file.commit())
        return error(DraftStoreError::Io, file.errorString());
    return {};
}

DraftStoreResult<DraftRecord> FileDraftStore::load(const QUuid &id) const
{
    const auto path = findPath(id);
    if (path.isEmpty())
        return { {}, error(DraftStoreError::NotFound, QStringLiteral("Draft was not found")) };
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return { {}, error(DraftStoreError::Io, file.errorString()) };
    auto plainText = decrypt(file.readAll());
    if (!plainText)
        return { {}, plainText.error };
    auto record = deserialize(plainText.value);
    if (record && record.value.id != id)
        return { {}, error(DraftStoreError::Corrupt, QStringLiteral("Draft identity mismatch")) };
    return record;
}

DraftStoreResult<QList<DraftRecord>> FileDraftStore::records() const
{
    QList<DraftRecord> result;
    QDir               dir(rootPath_);
    if (!dir.exists())
        return { result, {} };
    for (const auto &name : dir.entryList({ QStringLiteral("*.draft") }, QDir::Files)) {
        const QUuid id(QFileInfo(name).completeBaseName());
        if (id.isNull())
            return { {}, error(DraftStoreError::Corrupt, QStringLiteral("Invalid draft filename")) };
        auto record = load(id);
        if (!record)
            return { {}, record.error };
        result.push_back(record.value);
    }
    return { result, {} };
}

DraftStoreError FileDraftStore::transition(const QUuid &id, DraftRecord::State state)
{
    auto record = load(id);
    if (!record)
        return record.error;
    record.value.state     = state;
    record.value.updatedAt = QDateTime::currentDateTimeUtc();
    return write(record.value);
}

DraftStoreError FileDraftStore::remove(const QUuid &id)
{
    const auto path = findPath(id);
    if (path.isEmpty())
        return error(DraftStoreError::NotFound, QStringLiteral("Draft was not found"));
    if (!QFile::remove(path))
        return error(DraftStoreError::Io, QStringLiteral("Failed to remove draft"));
    return {};
}

} // namespace QtNote
