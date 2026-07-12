#include "xmppworker.h"

#include "qtnotepubsubitem.h"
#include "xmppnotecodec.h"
#include "xmppomemostorage.h"
#include "xmpppepextension.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>
#include <QXmppClient.h>
#include <QXmppConfiguration.h>
#include <QXmppDiscoveryIq.h>
#include <QXmppDiscoveryManager.h>
#include <QXmppE2eeMetadata.h>
#include <QXmppError.h>
#include <QXmppGlobal.h>
#include <QXmppMessage.h>
#include <QXmppOmemoManager.h>
#include <QXmppPubSubManager.h>
#include <QXmppPubSubNodeConfig.h>
#include <QXmppPubSubPublishOptions.h>
#include <QXmppStanza.h>
#include <QXmppTask.h>
#include <QXmppTrustManager.h>
#include <QXmppTrustMemoryStorage.h>
#include <QXmppUtils.h>

#include <optional>
#include <utility>
#include <variant>

namespace QtNote {

namespace {

    constexpr auto PublishOptionsFeature = "http://jabber.org/protocol/pubsub#publish-options";

    bool sameConfig(const XmppConfig &left, const XmppConfig &right)
    {
        return left.jid == right.jid && left.password == right.password && left.host == right.host
            && left.port == right.port && left.resource == right.resource && left.nodeName == right.nodeName
            && left.originId == right.originId && left.timeoutMs == right.timeoutMs && left.masterKey == right.masterKey
            && left.omemoStateKey == right.omemoStateKey && left.omemoStatePath == right.omemoStatePath;
    }

    template <typename T> std::optional<T> awaitTask(QXmppTask<T> &&task, int timeoutMs, QString *error)
    {
        QObject    guard;
        QEventLoop loop;
        QTimer     timer;
        timer.setSingleShot(true);

        std::optional<T> result;
        bool             timedOut = false;

        QObject::connect(&timer, &QTimer::timeout, &guard, [&]() {
            timedOut = true;
            loop.quit();
        });

        task.then(&guard, [&](T &&value) {
            result.emplace(std::move(value));
            loop.quit();
        });

        if (!result) {
            timer.start(qMax(1000, timeoutMs));
            loop.exec();
            timer.stop();
        }

        if (timedOut && error) {
            *error = QStringLiteral("XMPP operation timed out after %1 ms").arg(timeoutMs);
        }
        return result;
    }

    bool awaitVoidTask(QXmppTask<void> &&task, int timeoutMs, QString *error)
    {
        QObject    guard;
        QEventLoop loop;
        QTimer     timer;
        timer.setSingleShot(true);
        bool finished = false;
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        task.then(&guard, [&]() {
            finished = true;
            loop.quit();
        });
        if (!finished) {
            timer.start(qMax(1000, timeoutMs));
            loop.exec();
        }
        if (!finished && error)
            *error = QStringLiteral("XMPP operation timed out after %1 ms").arg(timeoutMs);
        return finished;
    }

    QXmppPubSubNodeConfig privateNodeConfig()
    {
        QXmppPubSubNodeConfig config;
        config.setAccessModel(QXmppPubSubNodeConfig::Allowlist);
        config.setPersistItems(true);
        config.setMaxItems(QXmppPubSubNodeConfig::Max {});
        config.setIncludePayloads(true);
        config.setRetractNotificationsEnabled(true);
        config.setNodeType(QXmppPubSubNodeConfig::Leaf);
        config.setPayloadType(QtNotePubSubItem::payloadNamespace);
        return config;
    }

    QXmppPubSubPublishOptions privatePublishOptions()
    {
        QXmppPubSubPublishOptions options;
        options.setAccessModel(QXmppPubSubNodeConfig::Allowlist);
        options.setPersistItems(true);
        return options;
    }

    bool nodeConfigIsPrivate(const QXmppPubSubNodeConfig &config)
    {
        return config.accessModel() == QXmppPubSubNodeConfig::Allowlist && config.persistItems().value_or(false);
    }

    template <typename Result> const QXmppError *resultError(const Result &result)
    {
        return std::get_if<QXmppError>(&result);
    }

    bool hasStanzaCondition(const QXmppError &error, QXmppStanza::Error::Condition condition)
    {
        const auto stanzaError = error.value<QXmppStanza::Error>();
        return stanzaError && stanzaError->condition() == condition;
    }

    bool isItemNotFound(const QXmppError &error)
    {
        return hasStanzaCondition(error, QXmppStanza::Error::ItemNotFound)
            || error.description.contains(QStringLiteral("No such item"), Qt::CaseInsensitive)
            || error.description.contains(QStringLiteral("item-not-found"), Qt::CaseInsensitive);
    }

} // namespace

XmppWorker::XmppWorker(QObject *parent) : QObject(parent) { qRegisterMetaType<XmppRemoteNote>(); }

XmppWorker::~XmppWorker()
{
    resetClient();
    delete trustStorage_;
    trustStorage_ = nullptr;
}

void XmppWorker::setConfig(const XmppConfig &config)
{
    if (sameConfig(config_, config)) {
        return;
    }
    config_ = config;
    resetClient();
}

void XmppWorker::shutdown() { resetClient(); }

void XmppWorker::resetClient()
{
    prepared_     = false;
    omemoReady_   = false;
    discovery_    = nullptr;
    pubSub_       = nullptr;
    pepExtension_ = nullptr;
    trustManager_ = nullptr;
    omemoManager_ = nullptr;
    pendingKeyRequests_.clear();
    pendingInboundKeyRequests_.clear();

    if (client_) {
        client_->disconnectFromServer();
        delete client_;
        client_ = nullptr;
    }
    delete omemoStorage_;
    omemoStorage_ = nullptr;
}

void XmppWorker::createClient()
{
    if (client_) {
        return;
    }

    client_       = new QXmppClient(QXmppClient::BasicExtensions, this);
    discovery_    = client_->findExtension<QXmppDiscoveryManager>();
    pubSub_       = client_->addNewExtension<QXmppPubSubManager>();
    pepExtension_ = client_->addNewExtension<XmppPepExtension>();
    omemoStorage_ = new XmppOmemoStorage(config_.omemoStatePath, config_.omemoStateKey, config_.jid);
    if (!trustStorage_)
        trustStorage_ = new QXmppTrustMemoryStorage;
    trustManager_ = client_->addNewExtension<QXmppTrustManager>(trustStorage_);
    omemoManager_ = client_->addNewExtension<QXmppOmemoManager>(omemoStorage_);
    pepExtension_->setOwnBareJid(config_.jid);
    pepExtension_->setNodeName(config_.indexNodeName());

    connect(pepExtension_, &XmppPepExtension::payloadPublished, this, [this](const XmppEncryptedPayload &payload) {
        auto note = XmppNoteCodec::decodeIndex(payload, config_.masterKey, config_.indexNodeName());
        if (note)
            emit remoteNotePublished(note.value);
        else
            emit workerError(note.error.message);
    });
    connect(pepExtension_, &XmppPepExtension::noteRetracted, this, &XmppWorker::remoteNoteRetracted);
    connect(pepExtension_, &XmppPepExtension::nodeInvalidated, this, &XmppWorker::remoteNodeInvalidated);
    connect(pepExtension_, &XmppPepExtension::malformedItem, this, &XmppWorker::workerError);

    connect(client_, &QXmppClient::connected, this, [this]() {
        prepared_ = false;
        emit connectionChanged(true);
    });
    connect(client_, &QXmppClient::disconnected, this, [this]() {
        prepared_ = false;
        emit connectionChanged(false);
    });
    connect(client_, &QXmppClient::errorOccurred, this,
            [this](const QXmppError &error) { emit workerError(errorText(error)); });
    connect(client_, &QXmppClient::messageReceived, this, &XmppWorker::handleKeySyncMessage);
}

XmppStatusResult XmppWorker::connectToServer()
{
    createClient();

    if (client_->isConnected()) {
        return { true };
    }

    QXmppConfiguration configuration;
    configuration.setJid(config_.jid);
    configuration.setPassword(config_.password);
    configuration.setResource(config_.resource);
    configuration.setAutoReconnectionEnabled(true);
    configuration.setStreamSecurityMode(QXmppConfiguration::TLSRequired);
    configuration.setIgnoreSslErrors(false);
    if (!config_.host.isEmpty()) {
        configuration.setHost(config_.host);
    }
    if (config_.port > 0) {
        configuration.setPort(config_.port);
    }

    QObject    guard;
    QEventLoop loop;
    QTimer     timer;
    timer.setSingleShot(true);

    bool    connected = false;
    QString connectionError;

    QObject::connect(client_, &QXmppClient::connected, &guard, [&]() {
        connected = true;
        loop.quit();
    });
    QObject::connect(client_, &QXmppClient::errorOccurred, &guard, [&](const QXmppError &error) {
        connectionError = errorText(error);
        loop.quit();
    });
    QObject::connect(client_, &QXmppClient::disconnected, &guard, [&]() {
        if (!connected && connectionError.isEmpty()) {
            connectionError = QStringLiteral("XMPP connection closed before authentication");
        }
        loop.quit();
    });
    QObject::connect(&timer, &QTimer::timeout, &guard, [&]() {
        connectionError = QStringLiteral("XMPP connection timed out after %1 ms").arg(config_.timeoutMs);
        loop.quit();
    });

    timer.start(qMax(1000, config_.timeoutMs));
    client_->connectToServer(configuration);
    if (!client_->isConnected()) {
        loop.exec();
    } else {
        connected = true;
    }
    timer.stop();

    if (!connected && !client_->isConnected()) {
        client_->disconnectFromServer();
        return { false, false, false,
                 connectionError.isEmpty() ? QStringLiteral("Failed to connect to the XMPP server") : connectionError };
    }

    return { true };
}

XmppStatusResult XmppWorker::verifyPrivateStorageSupport()
{
    if (!discovery_) {
        return { false, false, false, QStringLiteral("QXmpp discovery manager is unavailable") };
    }

    QString    waitError;
    const auto pepService = QXmppUtils::jidToBareJid(config_.jid);
#if QXMPP_VERSION >= QT_VERSION_CHECK(1, 12, 0)
    auto result = awaitTask(discovery_->info(pepService), config_.timeoutMs, &waitError);
#else
    auto result = awaitTask(discovery_->requestDiscoInfo(pepService), config_.timeoutMs, &waitError);
#endif
    if (!result) {
        return { false, false, false, waitError };
    }

    if (const auto *error = std::get_if<QXmppError>(&*result)) {
        return { false, false, false,
                 QStringLiteral("Could not discover the PEP service at %1: %2").arg(pepService, errorText(*error)) };
    }

    const auto &info           = std::get<0>(*result);
    bool        hasPepIdentity = false;
    for (const auto &identity : info.identities()) {
        if (identity.category() == QStringLiteral("pubsub") && identity.type() == QStringLiteral("pep")) {
            hasPepIdentity = true;
            break;
        }
    }

    if (!hasPepIdentity) {
        return { false, false, false, QStringLiteral("The XMPP server does not advertise a pubsub/pep identity") };
    }
    if (!info.features().contains(QString::fromLatin1(PublishOptionsFeature))) {
        return { false, false, false,
                 QStringLiteral("The server does not advertise PubSub publish-options; "
                                "QtNote will not store private notes there") };
    }

    return { true };
}

XmppStatusResult XmppWorker::ensureNode(const QString &nodeName)
{
    const auto verifyNode = [this, nodeName]() -> XmppStatusResult {
        QString waitError;
        auto    verify = awaitTask(pubSub_->requestOwnPepNodeConfiguration(nodeName), config_.timeoutMs, &waitError);
        if (!verify) {
            return { false, false, false, waitError };
        }
        if (const auto *error = std::get_if<QXmppError>(&*verify)) {
            return { false, false, false,
                     QStringLiteral("Could not verify the QtNote PEP node: %1").arg(errorText(*error)) };
        }

        const bool isPrivate = nodeConfigIsPrivate(std::get<QXmppPubSubNodeConfig>(*verify));

        if (!isPrivate) {
            return { false, false, false,
                     QStringLiteral("The QtNote PEP node is not persistent and private after configuration") };
        }
        return { true };
    };

    QString waitError;
    auto    request = awaitTask(pubSub_->requestOwnPepNodeConfiguration(nodeName), config_.timeoutMs, &waitError);
    if (!request) {
        return { false, false, false, waitError };
    }

    if (const auto *requestError = std::get_if<QXmppError>(&*request)) {
        if (!isItemNotFound(*requestError)) {
            return {
                false, false, false,
                QStringLiteral("Could not read the QtNote PEP node configuration: %1").arg(errorText(*requestError))
            };
        }

        auto create
            = awaitTask(pubSub_->createOwnPepNode(nodeName, privateNodeConfig()), config_.timeoutMs, &waitError);
        if (!create) {
            return { false, false, false, waitError };
        }
        if (const auto *error = resultError(*create)) {
            // Another resource can create the node between our configuration request
            // and this request. In that case verify the now-existing node.
            if (hasStanzaCondition(*error, QXmppStanza::Error::Conflict)) {
                return verifyNode();
            }
            return { false, false, false,
                     QStringLiteral("Could not create the private QtNote PEP node: %1").arg(errorText(*error)) };
        }
        return verifyNode();
    }

    auto config = std::get<QXmppPubSubNodeConfig>(*request);
    config.setAccessModel(QXmppPubSubNodeConfig::Allowlist);
    config.setPersistItems(true);
    config.setMaxItems(QXmppPubSubNodeConfig::Max {});
    config.setIncludePayloads(true);
    config.setRetractNotificationsEnabled(true);
    config.setNodeType(QXmppPubSubNodeConfig::Leaf);
    config.setPayloadType(QtNotePubSubItem::payloadNamespace);

    auto configured = awaitTask(pubSub_->configureOwnPepNode(nodeName, config), config_.timeoutMs, &waitError);
    if (!configured) {
        return { false, false, false, waitError };
    }
    if (const auto *error = resultError(*configured)) {
        return { false, false, false,
                 QStringLiteral("Could not configure the private QtNote PEP node: %1").arg(errorText(*error)) };
    }

    return verifyNode();
}

XmppStatusResult XmppWorker::ensureReady()
{
    auto connected = connectToServer();
    if (!connected.ok) {
        return connected;
    }
    auto omemo = ensureOmemo();
    if (!omemo.ok)
        return omemo;
    if (prepared_) {
        return { true };
    }

    auto support = verifyPrivateStorageSupport();
    if (!support.ok) {
        return support;
    }

    auto node = ensureNode(config_.indexNodeName());
    if (!node.ok)
        return node;
    node = ensureNode(config_.contentNodeName());
    if (!node.ok)
        return node;

    prepared_ = true;
    return { true };
}

XmppStatusResult XmppWorker::ensureOmemo()
{
    if (omemoReady_)
        return { true };
    if (!omemoStorage_ || !omemoStorage_->isValid())
        return { false, false, false,
                 omemoStorage_ ? omemoStorage_->errorString() : QStringLiteral("OMEMO storage is unavailable") };
    omemoManager_->setAcceptedSessionBuildingTrustLevels(QXmpp::TrustLevel::ManuallyTrusted
                                                         | QXmpp::TrustLevel::Authenticated);
    omemoManager_->setNewDeviceAutoSessionBuildingEnabled(false);
    QString waitError;
    auto    loaded = awaitTask(omemoManager_->load(), config_.timeoutMs, &waitError);
    if (!loaded)
        return { false, false, false, waitError };
    if (!*loaded) {
        auto setup = awaitTask(omemoManager_->setUp(config_.resource), config_.timeoutMs, &waitError);
        if (!setup)
            return { false, false, false, waitError };
        if (!*setup)
            return { false, false, false, QStringLiteral("Could not initialize the OMEMO device") };
    }
    omemoReady_ = true;
    return { true };
}

XmppStatusResult XmppWorker::probe() { return ensureReady(); }

XmppStatusResult XmppWorker::requestStorageKey()
{
    auto connected = connectToServer();
    if (!connected.ok)
        return connected;
    auto omemo = ensureOmemo();
    if (!omemo.ok)
        return omemo;
    const auto requestId = newUuid();
    pendingKeyRequests_.insert(requestId);
    QJsonObject request { { QStringLiteral("protocol"), QStringLiteral("urn:xmpp:qtnote:key-sync:1") },
                          { QStringLiteral("type"), QStringLiteral("request") },
                          { QStringLiteral("requestId"), requestId } };
    sendKeySyncMessage(QXmppUtils::jidToBareJid(config_.jid), request);
    return { true };
}

QList<XmppDeviceInfo> XmppWorker::ownOmemoDevices(QString *error)
{
    auto connected = connectToServer();
    if (!connected.ok) {
        if (error)
            *error = connected.error;
        return {};
    }
    auto omemo = ensureOmemo();
    if (!omemo.ok) {
        if (error)
            *error = omemo.error;
        return {};
    }
    QString waitError;
    auto    devices
        = awaitTask(omemoManager_->devices({ QXmppUtils::jidToBareJid(config_.jid) }), config_.timeoutMs, &waitError);
    if (!devices) {
        if (error)
            *error = waitError;
        return {};
    }
    QList<XmppDeviceInfo> result;
    for (const auto &device : *devices)
        result.append({ device.label(), device.keyId(), int(device.trustLevel()) });
    return result;
}

XmppStatusResult XmppWorker::trustOwnOmemoDevice(const QByteArray &keyId)
{
    if (keyId.isEmpty())
        return { false, false, false, QStringLiteral("No OMEMO device was selected") };
    QString    error;
    const auto devices       = ownOmemoDevices(&error);
    const bool belongsToSelf = std::any_of(devices.cbegin(), devices.cend(),
                                           [&keyId](const XmppDeviceInfo &device) { return device.keyId == keyId; });
    if (!belongsToSelf)
        return { false, false, false,
                 error.isEmpty() ? QStringLiteral("The OMEMO key does not belong to an own device") : error };
    QMultiHash<QString, QByteArray> keys;
    keys.insert(QXmppUtils::jidToBareJid(config_.jid), keyId);
    if (!awaitVoidTask(omemoManager_->setTrustLevel(keys, QXmpp::TrustLevel::ManuallyTrusted), config_.timeoutMs,
                       &error))
        return { false, false, false, error };
    if (!awaitVoidTask(omemoManager_->buildMissingSessions({ QXmppUtils::jidToBareJid(config_.jid) }),
                       config_.timeoutMs, &error))
        return { false, false, false, error };
    return { true };
}

void XmppWorker::approveKeySyncRequest(const QString &requestId)
{
    const auto pending = pendingInboundKeyRequests_.take(requestId);
    if (pending.senderKey.isEmpty())
        return;
    const auto trusted = trustOwnOmemoDevice(pending.senderKey);
    if (!trusted.ok) {
        emit workerError(trusted.error);
        return;
    }
    if (config_.masterKey.size() != SecureEnvelope::MasterKeySize)
        return;
    QJsonObject response { { QStringLiteral("protocol"), QStringLiteral("urn:xmpp:qtnote:key-sync:1") },
                           { QStringLiteral("type"), QStringLiteral("response") },
                           { QStringLiteral("requestId"), requestId },
                           { QStringLiteral("recoveryKey"), SecureEnvelope::encodeRecoveryKey(config_.masterKey) } };
    sendKeySyncMessage(pending.from, response);
}

void XmppWorker::sendKeySyncMessage(const QString &to, const QJsonObject &payload)
{
    QXmppMessage message;
    message.setTo(to);
    message.setType(QXmppMessage::Chat);
    message.setBody(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact)));
    message.setE2eeFallbackBody(QStringLiteral("QtNote encrypted storage-key synchronization message"));
    client_->sendSensitive(std::move(message)).then(client_, [this](QXmpp::SendResult result) {
        if (const auto *error = std::get_if<QXmppError>(&result))
            emit workerError(QStringLiteral("Could not send the OMEMO key-sync message: %1").arg(errorText(*error)));
    });
}

void XmppWorker::handleKeySyncMessage(const QXmppMessage &message)
{
    if (QXmppUtils::jidToBareJid(message.from()) != QXmppUtils::jidToBareJid(config_.jid))
        return;
    const auto metadata = message.e2eeMetadata();
    if (!metadata
        || (metadata->encryption() != QXmpp::Omemo0 && metadata->encryption() != QXmpp::Omemo1
            && metadata->encryption() != QXmpp::Omemo2))
        return;
    QJsonParseError parseError;
    const auto      document = QJsonDocument::fromJson(message.body().toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return;
    const auto payload = document.object();
    if (payload.value(QStringLiteral("protocol")).toString() != QStringLiteral("urn:xmpp:qtnote:key-sync:1"))
        return;

    QString waitError;
    auto    trust = awaitTask(omemoManager_->trustLevel(QXmppUtils::jidToBareJid(config_.jid), metadata->senderKey()),
                              config_.timeoutMs, &waitError);
    if (!trust) {
        emit workerError(waitError);
        return;
    }
    const auto type = payload.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("request")) {
        const auto requestId = payload.value(QStringLiteral("requestId")).toString();
        if (requestId.isEmpty())
            return;
        if (*trust != QXmpp::TrustLevel::ManuallyTrusted && *trust != QXmpp::TrustLevel::Authenticated) {
            const auto devices   = ownOmemoDevices(&waitError);
            const bool ownDevice = std::any_of(devices.cbegin(), devices.cend(), [&](const XmppDeviceInfo &device) {
                return device.keyId == metadata->senderKey();
            });
            if (!ownDevice) {
                emit workerError(waitError.isEmpty()
                                     ? QStringLiteral("Ignored a storage-key request from an unknown OMEMO device")
                                     : waitError);
                return;
            }
            pendingInboundKeyRequests_.insert(requestId, { message.from(), metadata->senderKey() });
            emit keySyncTrustRequested(requestId, metadata->senderKey());
            return;
        }
        if (config_.masterKey.size() != SecureEnvelope::MasterKeySize)
            return;
        QJsonObject response { { QStringLiteral("protocol"), QStringLiteral("urn:xmpp:qtnote:key-sync:1") },
                               { QStringLiteral("type"), QStringLiteral("response") },
                               { QStringLiteral("requestId"), payload.value(QStringLiteral("requestId")) },
                               { QStringLiteral("recoveryKey"),
                                 SecureEnvelope::encodeRecoveryKey(config_.masterKey) } };
        sendKeySyncMessage(message.from(), response);
    } else if (type == QStringLiteral("response")) {
        if (*trust != QXmpp::TrustLevel::ManuallyTrusted && *trust != QXmpp::TrustLevel::Authenticated)
            return;
        const auto requestId = payload.value(QStringLiteral("requestId")).toString();
        if (!pendingKeyRequests_.remove(requestId))
            return;
        auto key = SecureEnvelope::decodeRecoveryKey(payload.value(QStringLiteral("recoveryKey")).toString());
        if (!key) {
            emit workerError(QStringLiteral("Received an invalid XMPP storage key: %1").arg(key.error.message));
            return;
        }
        emit storageKeyReceived(key.value);
    }
}

XmppListResult XmppWorker::listNotes()
{
    XmppListResult output;
    const auto     ready = ensureReady();
    if (!ready.ok) {
        output.error = ready.error;
        return output;
    }

    QString waitError;
    auto idsResult = awaitTask(pubSub_->requestOwnPepItemIds(config_.indexNodeName()), config_.timeoutMs, &waitError);

    if (idsResult && std::holds_alternative<QVector<QString>>(*idsResult)) {
        const auto    ids       = std::get<QVector<QString>>(*idsResult);
        constexpr int BatchSize = 50;

        for (int offset = 0; offset < ids.size(); offset += BatchSize) {
            QStringList batch;
            const int   end = qMin(offset + BatchSize, ids.size());
            for (int i = offset; i < end; ++i) {
                batch.append(ids.at(i));
            }

            auto itemsResult = awaitTask(pubSub_->requestItems<QtNotePubSubItem>(QXmppUtils::jidToBareJid(config_.jid),
                                                                                 config_.indexNodeName(), batch),
                                         config_.timeoutMs, &waitError);
            if (!itemsResult) {
                output.error = waitError;
                return output;
            }
            if (const auto *error = std::get_if<QXmppError>(&*itemsResult)) {
                output.error = errorText(*error);
                return output;
            }

            const auto &items = std::get<QXmppPubSubManager::Items<QtNotePubSubItem>>(*itemsResult).items;
            for (const auto &item : items) {
                if (!item.isValid()) {
                    output.error = item.parseError();
                    return output;
                }
                auto note = XmppNoteCodec::decodeIndex(item.payload(), config_.masterKey, config_.indexNodeName());
                if (!note) {
                    output.error = note.error.message;
                    return output;
                }
                output.notes.append(note.value);
            }
        }

        output.ok = true;
        return output;
    }

    // Compatibility fallback for servers that do not expose item IDs through disco#items.
    auto itemsResult = awaitTask(
        pubSub_->requestItems<QtNotePubSubItem>(QXmppUtils::jidToBareJid(config_.jid), config_.indexNodeName()),
        config_.timeoutMs, &waitError);
    if (!itemsResult) {
        output.error = waitError;
        return output;
    }
    if (const auto *error = std::get_if<QXmppError>(&*itemsResult)) {
        output.error = errorText(*error);
        return output;
    }

    const auto &items = std::get<QXmppPubSubManager::Items<QtNotePubSubItem>>(*itemsResult);
    output.partial    = items.continuation.has_value();
    for (const auto &item : items.items) {
        if (!item.isValid()) {
            output.error = item.parseError();
            return output;
        }
        auto note = XmppNoteCodec::decodeIndex(item.payload(), config_.masterKey, config_.indexNodeName());
        if (!note) {
            output.error = note.error.message;
            return output;
        }
        output.notes.append(note.value);
    }
    output.ok = true;
    return output;
}

XmppNoteResult XmppWorker::requestNote(const QString &id)
{
    XmppNoteResult output;
    QString        waitError;
    auto           indexResult = awaitTask(
        pubSub_->requestItem<QtNotePubSubItem>(QXmppUtils::jidToBareJid(config_.jid), config_.indexNodeName(), id),
        config_.timeoutMs, &waitError);
    if (!indexResult) {
        output.error = waitError;
        return output;
    }
    if (const auto *error = std::get_if<QXmppError>(&*indexResult)) {
        output.error    = errorText(*error);
        output.notFound = isItemNotFound(*error);
        return output;
    }
    const auto &indexItem = std::get<QtNotePubSubItem>(*indexResult);
    if (!indexItem.isValid()) {
        output.error = indexItem.parseError();
        return output;
    }
    auto index = XmppNoteCodec::decodeIndex(indexItem.payload(), config_.masterKey, config_.indexNodeName());
    if (!index) {
        output.error = index.error.message;
        return output;
    }
    auto contentResult = awaitTask(
        pubSub_->requestItem<QtNotePubSubItem>(QXmppUtils::jidToBareJid(config_.jid), config_.contentNodeName(), id),
        config_.timeoutMs, &waitError);
    if (!contentResult) {
        output.error = waitError;
        return output;
    }
    if (const auto *error = std::get_if<QXmppError>(&*contentResult)) {
        output.error = errorText(*error);
        return output;
    }
    const auto &contentItem = std::get<QtNotePubSubItem>(*contentResult);
    if (!contentItem.isValid()) {
        output.error = contentItem.parseError();
        return output;
    }
    auto content = XmppNoteCodec::decodeContent(contentItem.payload(), config_.masterKey, config_.contentNodeName(),
                                                index.value);
    if (!content) {
        output.error = content.error.message;
        return output;
    }
    output.note                = index.value;
    output.note.content        = content.value;
    output.note.contentPresent = true;
    output.ok                  = true;
    return output;
}

XmppNoteResult XmppWorker::getNote(const QString &id)
{
    XmppNoteResult output;
    const auto     ready = ensureReady();
    if (!ready.ok) {
        output.error = ready.error;
        return output;
    }
    return requestNote(id);
}

XmppNoteResult XmppWorker::saveNote(const XmppRemoteNote &localNote)
{
    XmppNoteResult output;
    const auto     ready = ensureReady();
    if (!ready.ok) {
        output.error = ready.error;
        return output;
    }

    XmppRemoteNote updated = localNote;
    if (updated.id.isEmpty()) {
        updated.id = newUuid();
    } else {
        const auto server = requestNote(updated.id);
        if (!server.ok) {
            output.error    = server.error;
            output.notFound = server.notFound;
            return output;
        }
        if (server.note.revision != updated.revision) {
            output.conflict         = true;
            output.remoteOnConflict = server.note;
            output.error
                = QStringLiteral("The note was modified on another XMPP resource; the local version was not published");
            return output;
        }
    }

    updated.parentRevision = updated.revision;
    updated.revision       = newUuid();
    updated.originId       = config_.originId;
    updated.modified       = QDateTime::currentDateTimeUtc();
    updated.format         = QStringLiteral("markdown");
    updated.contentPresent = true;

    auto contentPayload = XmppNoteCodec::encodeContent(updated, config_.masterKey, config_.contentNodeName());
    auto indexPayload   = XmppNoteCodec::encodeIndex(updated, config_.masterKey, config_.indexNodeName());
    if (!contentPayload || !indexPayload) {
        output.error = !contentPayload ? contentPayload.error.message : indexPayload.error.message;
        return output;
    }
    QString waitError;
    auto    contentPublish
        = awaitTask(pubSub_->publishOwnPepItem(config_.contentNodeName(), QtNotePubSubItem(contentPayload.value),
                                               privatePublishOptions()),
                    config_.timeoutMs, &waitError);
    if (!contentPublish) {
        output.error = waitError;
        return output;
    }
    if (const auto *error = std::get_if<QXmppError>(&*contentPublish)) {
        output.error = errorText(*error);
        return output;
    }
    auto indexPublish
        = awaitTask(pubSub_->publishOwnPepItem(config_.indexNodeName(), QtNotePubSubItem(indexPayload.value),
                                               privatePublishOptions()),
                    config_.timeoutMs, &waitError);
    if (!indexPublish) {
        output.error = waitError;
        return output;
    }
    if (const auto *error = std::get_if<QXmppError>(&*indexPublish)) {
        output.error = errorText(*error);
        return output;
    }

    output.note = updated;
    output.ok   = true;
    return output;
}

XmppStatusResult XmppWorker::deleteNote(const QString &id)
{
    const auto ready = ensureReady();
    if (!ready.ok) {
        return ready;
    }

    QString waitError;
    auto    indexResult
        = awaitTask(pubSub_->retractItem(QXmppUtils::jidToBareJid(config_.jid), config_.indexNodeName(), id, true),
                    config_.timeoutMs, &waitError);
    if (!indexResult) {
        return { false, false, false, waitError };
    }
    if (const auto *error = resultError(*indexResult)) {
        const auto text     = errorText(*error);
        const bool notFound = isItemNotFound(*error);
        if (!notFound)
            return { false, false, false, text };
    }
    auto contentResult
        = awaitTask(pubSub_->retractItem(QXmppUtils::jidToBareJid(config_.jid), config_.contentNodeName(), id, true),
                    config_.timeoutMs, &waitError);
    if (!contentResult)
        return { false, false, false, waitError };
    if (const auto *error = resultError(*contentResult)) {
        const bool notFound = isItemNotFound(*error);
        if (!notFound)
            return { false, false, false, errorText(*error) };
    }
    return { true };
}

QString XmppWorker::newUuid() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }

QString XmppWorker::errorText(const QXmppError &error)
{
    return error.description.isEmpty() ? QStringLiteral("Unknown XMPP error") : error.description;
}

} // namespace QtNote
