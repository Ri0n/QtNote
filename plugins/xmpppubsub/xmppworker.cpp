#include "xmppworker.h"

#include "qtnotepubsubitem.h"
#include "xmppkeysyncextension.h"
#include "xmppnotecodec.h"
#include "xmppomemopubsubitems.h"
#include "xmppomemostorage.h"
#include "xmpppepextension.h"
#include "xmpppersistenttruststorage.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimer>
#include <QUuid>
#include <QXmppClient.h>
#include <QXmppConfiguration.h>
#include <QXmppDiscoveryIq.h>
#include <QXmppDiscoveryManager.h>
#include <QXmppE2eeMetadata.h>
#include <QXmppError.h>
#include <QXmppGlobal.h>
#include <QXmppLogger.h>
#include <QXmppOmemoManager.h>
#include <QXmppPubSubManager.h>
#include <QXmppPubSubNodeConfig.h>
#include <QXmppPubSubPublishOptions.h>
#include <QXmppRosterManager.h>
#include <QXmppStanza.h>
#include <QXmppTask.h>
#include <QXmppTrustManager.h>
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

    QString sanitizedXmppLog(QString xml)
    {
        if (xml.contains(QStringLiteral("<bundle")) || xml.contains(QStringLiteral("<prekeys"))
            || xml.contains(QRegularExpression(QStringLiteral("<pk\\s")))) {
            const auto itemMatch = QRegularExpression(QStringLiteral("<item\\s[^>]*id=['\"]([^'\"]+)['\"]")).match(xml);
            const auto identityMatch = QRegularExpression(QStringLiteral("<ik\\b[^>]*>([^<]+)</ik>")).match(xml);
            QStringList details;
            if (itemMatch.hasMatch())
                details.append(QStringLiteral("device=%1").arg(itemMatch.captured(1)));
            if (identityMatch.hasMatch())
                details.append(QStringLiteral("identity-key=%1").arg(identityMatch.captured(1)));
            return details.isEmpty()
                ? QStringLiteral("[OMEMO public prekeys omitted]")
                : QStringLiteral("[OMEMO bundle %1; public prekeys omitted]").arg(details.join(QLatin1Char(' ')));
        }
        if (xml.contains(QStringLiteral("<encrypted"))) {
            const auto sid
                = QRegularExpression(QStringLiteral("<header\\s[^>]*sid=['\"]([^'\"]+)['\"]")).match(xml).captured(1);
            QStringList recipients;
            auto        keys = QRegularExpression(QStringLiteral("<key\\s([^>]*)>")).globalMatch(xml);
            while (keys.hasNext()) {
                const auto attributes = keys.next().captured(1);
                const auto rid
                    = QRegularExpression(QStringLiteral("rid=['\"]([^'\"]+)['\"]")).match(attributes).captured(1);
                const bool kex = QRegularExpression(QStringLiteral("kex=['\"]true['\"]")).match(attributes).hasMatch();
                if (!rid.isEmpty())
                    recipients.append(QStringLiteral("%1%2").arg(rid, kex ? QStringLiteral("/kex") : QString {}));
            }
            return QStringLiteral("[OMEMO encrypted sid=%1 recipients=%2]")
                .arg(sid.isEmpty() ? QStringLiteral("?") : sid,
                     recipients.isEmpty() ? QStringLiteral("?") : recipients.join(QLatin1Char(',')));
        }
        static const QRegularExpression sensitiveElement(
            QStringLiteral("(<(?:auth|response|encrypted)\\b[^>]*>).*?(</(?:auth|response|encrypted)>)"),
            QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption);
        xml.replace(sensitiveElement, QStringLiteral("\\1[redacted]\\2"));
        return xml;
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

XmppWorker::~XmppWorker() { resetClient(); }

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
    prepared_         = false;
    omemoReady_       = false;
    discovery_        = nullptr;
    roster_           = nullptr;
    pubSub_           = nullptr;
    pepExtension_     = nullptr;
    keySyncExtension_ = nullptr;
    trustManager_     = nullptr;
    omemoManager_     = nullptr;
    pendingInboundKeyRequests_.clear();

    if (client_) {
        client_->disconnectFromServer();
        delete client_;
        client_ = nullptr;
    }
    delete trustStorage_;
    trustStorage_ = nullptr;
    delete omemoStorage_;
    omemoStorage_ = nullptr;
}

void XmppWorker::createClient()
{
    if (client_) {
        return;
    }

    client_ = new QXmppClient(QXmppClient::BasicExtensions, this);
    if (qEnvironmentVariableIntValue("QTNOTE_XMPP_XML_LOG") > 0) {
        auto *logger = new QXmppLogger(client_);
        logger->setMessageTypes(QXmppLogger::SentMessage | QXmppLogger::ReceivedMessage | QXmppLogger::WarningMessage);
        logger->setLoggingType(QXmppLogger::SignalLogging);
        connect(logger, &QXmppLogger::message, this, [](QXmppLogger::MessageType type, const QString &message) {
            const auto direction = type == QXmppLogger::SentMessage ? QStringLiteral("XMPP >>")
                : type == QXmppLogger::ReceivedMessage              ? QStringLiteral("XMPP <<")
                                                                    : QStringLiteral("XMPP !!");
            qInfo().noquote() << direction << sanitizedXmppLog(message);
        });
        client_->setLogger(logger);
    }
    discovery_        = client_->findExtension<QXmppDiscoveryManager>();
    roster_           = client_->findExtension<QXmppRosterManager>();
    pubSub_           = client_->addNewExtension<QXmppPubSubManager>();
    pepExtension_     = client_->addNewExtension<XmppPepExtension>();
    keySyncExtension_ = client_->addNewExtension<XmppKeySyncExtension>();
    omemoStorage_     = new XmppOmemoStorage(config_.omemoStatePath, config_.omemoStateKey, config_.jid);
    if (!trustStorage_) {
        auto *persistentTrust = new XmppPersistentTrustStorage(config_.omemoStatePath + QStringLiteral(".trust"),
                                                               config_.omemoStateKey, config_.jid);
        if (!persistentTrust->isValid())
            qWarning().noquote() << "Could not load persistent OMEMO trust:" << persistentTrust->errorString();
        trustStorage_ = persistentTrust;
    }
    trustManager_ = client_->addNewExtension<QXmppTrustManager>(trustStorage_);
    omemoManager_ = client_->addNewExtension<QXmppOmemoManager>(omemoStorage_);
    client_->setEncryptionExtension(omemoManager_);
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
    connect(keySyncExtension_, &XmppKeySyncExtension::requestReceived, this, &XmppWorker::handleKeySyncRequest);

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
    if (nodeConfigIsPrivate(config))
        return { true };
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

QStringList XmppWorker::onlineQtNoteResources(QString *error)
{
    if (!roster_ || !discovery_) {
        if (error)
            *error = QStringLiteral("XMPP resource discovery is unavailable");
        return {};
    }
    const auto  bareJid     = QXmppUtils::jidToBareJid(config_.jid);
    const auto  ownResource = client_->configuration().resource();
    QStringList result;
    QStringList failures;
    QString     waitError;
    for (const auto &resource : roster_->getResources(bareJid)) {
        if (resource.isEmpty() || resource == ownResource)
            continue;
        const auto fullJid = bareJid + QLatin1Char('/') + resource;
#if QXMPP_VERSION >= QT_VERSION_CHECK(1, 12, 0)
        auto info = awaitTask(discovery_->info(fullJid), config_.timeoutMs, &waitError);
#else
        auto info = awaitTask(discovery_->requestDiscoInfo(fullJid), config_.timeoutMs, &waitError);
#endif
        if (!info) {
            failures.append(QStringLiteral("%1: %2").arg(fullJid, waitError));
            continue;
        }
        if (const auto *requestError = std::get_if<QXmppError>(&*info)) {
            failures.append(QStringLiteral("%1: %2").arg(fullJid, errorText(*requestError)));
            continue;
        }
        const auto &disco = std::get<0>(*info);
        if (!disco.features().contains(XmppKeySyncExtension::feature))
            continue;
        result.append(resource);
    }
    if (error && !failures.isEmpty())
        *error = failures.join(QLatin1Char('\n'));
    return result;
}

XmppStatusResult XmppWorker::probe() { return ensureReady(); }

XmppKeyAuditResult XmppWorker::auditStorageKeys()
{
    XmppKeyAuditResult output;
    auto               connected = connectToServer();
    if (!connected.ok) {
        output.error = connected.error;
        return output;
    }
    auto omemo = ensureOmemo();
    if (!omemo.ok) {
        output.error = omemo.error;
        return output;
    }
    // Destroy QXmpp's in-memory session cache before editing persistent storage. Otherwise the
    // old manager can write its stale sessions back while it is being destroyed.
    resetClient();
    QString sessionResetError;
    {
        XmppOmemoStorage cleanStorage(config_.omemoStatePath, config_.omemoStateKey, config_.jid);
        if (!cleanStorage.isValid()) {
            output.error = cleanStorage.errorString();
            return output;
        }
        if (!awaitVoidTask(cleanStorage.removeDevices(QXmppUtils::jidToBareJid(config_.jid)), config_.timeoutMs,
                           &sessionResetError)) {
            output.error = QStringLiteral("Could not reset cached OMEMO sessions: %1").arg(sessionResetError);
            return output;
        }
    }
    auto reconnected = connectToServer();
    if (!reconnected.ok) {
        output.error = reconnected.error;
        return output;
    }
    omemo = ensureOmemo();
    if (!omemo.ok) {
        output.error = omemo.error;
        return output;
    }
    if (client_->encryptionExtension() != omemoManager_) {
        output.error = QStringLiteral("OMEMO is not installed as the XMPP encryption extension; refusing to send "
                                      "the key-sync request without encryption");
        return output;
    }
    if (!keySyncExtension_) {
        output.error = QStringLiteral("XMPP resource discovery is unavailable");
        return output;
    }

    if (config_.masterKey.size() == SecureEnvelope::MasterKeySize) {
        output.candidates.append({ client_->configuration().resource(), config_.masterKey,
                                   SecureEnvelope::keyId(config_.masterKey), 0, true });
    }

    const auto  bareJid = QXmppUtils::jidToBareJid(config_.jid);
    QString     discoveryError;
    const auto  resources = onlineQtNoteResources(&discoveryError);
    QStringList qtNoteResources;
    for (const auto &resource : resources)
        qtNoteResources.append(bareJid + QLatin1Char('/') + resource);
    QString     waitError;
    QStringList errors;
    if (!discoveryError.isEmpty())
        errors.append(discoveryError);
    for (const auto &resource : qtNoteResources) {
        const auto requestId = newUuid();
        auto       result = awaitTask(client_->sendSensitiveIq(keySyncExtension_->makeRequest(resource, requestId), {}),
                                      config_.timeoutMs, &waitError);
        if (!result) {
            errors.append(QStringLiteral("%1: %2").arg(resource, waitError));
            continue;
        }
        if (const auto *error = std::get_if<QXmppError>(&*result)) {
            errors.append(QStringLiteral("%1: %2").arg(resource, errorText(*error)));
            continue;
        }
        const auto encoded = XmppKeySyncExtension::responseRecoveryKey(std::get<QDomElement>(*result), requestId);
        auto       key     = SecureEnvelope::decodeRecoveryKey(encoded);
        if (!key) {
            errors.append(QStringLiteral("%1: invalid storage key response").arg(resource));
            continue;
        }
        const auto keyId    = SecureEnvelope::keyId(key.value);
        auto       existing = std::find_if(output.candidates.begin(), output.candidates.end(),
                                           [&](const auto &candidate) { return candidate.keyId == keyId; });
        if (existing == output.candidates.end())
            output.candidates.append({ resource, key.value, keyId, 0, false });
        else if (!existing->resource.split(QStringLiteral(", ")).contains(resource))
            existing->resource += QStringLiteral(", ") + resource;
    }

    QString waitError2;
    auto idsResult = awaitTask(pubSub_->requestOwnPepItemIds(config_.indexNodeName()), config_.timeoutMs, &waitError2);
    if (!idsResult || std::holds_alternative<QXmppError>(*idsResult)) {
        output.error = !idsResult ? waitError2 : errorText(std::get<QXmppError>(*idsResult));
        return output;
    }
    const auto ids          = std::get<QVector<QString>>(*idsResult);
    output.totalIndexItems  = ids.size();
    constexpr int BatchSize = 50;
    for (int offset = 0; offset < ids.size(); offset += BatchSize) {
        QStringList batch;
        for (int i = offset; i < qMin(offset + BatchSize, ids.size()); ++i)
            batch.append(ids.at(i));
        auto items = awaitTask(pubSub_->requestItems<QtNotePubSubItem>(QXmppUtils::jidToBareJid(config_.jid),
                                                                       config_.indexNodeName(), batch),
                               config_.timeoutMs, &waitError2);
        if (!items || std::holds_alternative<QXmppError>(*items)) {
            output.error = !items ? waitError2 : errorText(std::get<QXmppError>(*items));
            return output;
        }
        for (const auto &item : std::get<QXmppPubSubManager::Items<QtNotePubSubItem>>(*items).items) {
            if (!item.isValid()) {
                output.error = item.parseError();
                return output;
            }
            const auto keyId     = item.payload().keyId;
            auto       candidate = std::find_if(output.candidates.begin(), output.candidates.end(),
                                                [&](const auto &entry) { return entry.keyId == keyId; });
            if (candidate == output.candidates.end())
                output.candidates.append({ {}, {}, keyId, 1, false });
            else
                ++candidate->indexItemCount;
        }
    }
    output.ok = true;
    if (!errors.isEmpty())
        output.error = QStringLiteral("Some QtNote resources failed:\n%1").arg(errors.join('\n'));
    return output;
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
    auto    deviceLists = awaitTask(omemoManager_->requestDeviceLists({ QXmppUtils::jidToBareJid(config_.jid) }),
                                    config_.timeoutMs, &waitError);
    if (!deviceLists) {
        if (error)
            *error = waitError;
        return {};
    }
    for (const auto &deviceList : *deviceLists) {
        if (const auto *requestError = std::get_if<QXmppError>(&deviceList.result)) {
            if (error)
                *error = errorText(*requestError);
            return {};
        }
    }
    const auto bareJid = QXmppUtils::jidToBareJid(config_.jid);
    auto       list    = awaitTask(pubSub_->requestItem<XmppOmemoDeviceListItem>(
                              bareJid, QStringLiteral("urn:xmpp:omemo:2:devices"), QStringLiteral("current")),
                                   config_.timeoutMs, &waitError);
    if (!list || std::holds_alternative<QXmppError>(*list)) {
        if (error)
            *error = !list ? waitError : errorText(std::get<QXmppError>(*list));
        return {};
    }
    const auto            ownDeviceId = omemoStorage_->ownDeviceId();
    const auto            ownKey      = omemoStorage_->ownIdentityKey();
    QList<XmppDeviceInfo> result;
    int                   missingFingerprints = 0;
    for (const auto &listed : std::get<XmppOmemoDeviceListItem>(*list).devices()) {
        if (listed.id.toUInt() == ownDeviceId)
            continue;
        auto bundle = awaitTask(
            pubSub_->requestItem<XmppOmemoBundleItem>(bareJid, QStringLiteral("urn:xmpp:omemo:2:bundles"), listed.id),
            config_.timeoutMs, &waitError);
        QByteArray keyId;
        if (!bundle) {
            qWarning().noquote() << "Could not fetch OMEMO bundle: id=" << listed.id << "label=" << listed.label
                                 << "error=" << waitError;
        } else if (const auto *requestError = std::get_if<QXmppError>(&*bundle)) {
            qWarning().noquote() << "Could not fetch OMEMO bundle: id=" << listed.id << "label=" << listed.label
                                 << "error=" << errorText(*requestError);
        } else {
            keyId = std::get<XmppOmemoBundleItem>(*bundle).identityKey();
            qInfo().noquote() << "Parsed OMEMO bundle: id=" << listed.id << "label=" << listed.label
                              << "identity-key-size=" << keyId.size() << "identity-key="
                              << (keyId.isEmpty() ? QStringLiteral("<missing>") : QString::fromLatin1(keyId.toHex()));
        }
        if (!keyId.isEmpty() && keyId == ownKey)
            continue;
        const auto deviceName = listed.label.isEmpty() ? QStringLiteral("OMEMO device %1").arg(listed.id)
                                                       : QStringLiteral("%1 (OMEMO %2)").arg(listed.label, listed.id);
        if (keyId.isEmpty()) {
            ++missingFingerprints;
            result.append({ deviceName, listed.id.toUInt(), {}, int(QXmpp::TrustLevel::Undecided) });
            continue;
        }
        auto trust = awaitTask(omemoManager_->trustLevel(bareJid, keyId), config_.timeoutMs, &waitError);
        result.append({ deviceName, listed.id.toUInt(), keyId, int(trust ? *trust : QXmpp::TrustLevel::Undecided) });
    }
    if (error && missingFingerprints > 0) {
        *error = QStringLiteral("Could not obtain the OMEMO fingerprint for %1 device(s)").arg(missingFingerprints);
    }
    return result;
}

XmppDeviceInfo XmppWorker::ownOmemoDevice() const
{
    if (!omemoStorage_)
        return {};
    return { omemoStorage_->ownDeviceLabel(), omemoStorage_->ownDeviceId(), omemoStorage_->ownIdentityKey(),
             int(QXmpp::TrustLevel::Authenticated) };
}

bool XmppWorker::ownOmemoBundleValid(QString *error)
{
    const auto connected = connectToServer();
    if (!connected.ok) {
        if (error)
            *error = connected.error;
        return false;
    }
    const auto omemo = ensureOmemo();
    if (!omemo.ok) {
        if (error)
            *error = omemo.error;
        return false;
    }
    const auto own = ownOmemoDevice();
    if (!own.deviceId || own.keyId.isEmpty()) {
        if (error)
            *error = QStringLiteral("The local OMEMO device is not initialized");
        return false;
    }
    QString waitError;
    auto    bundle = awaitTask(pubSub_->requestItem<XmppOmemoBundleItem>(QXmppUtils::jidToBareJid(config_.jid),
                                                                         QStringLiteral("urn:xmpp:omemo:2:bundles"),
                                                                         QString::number(own.deviceId)),
                               config_.timeoutMs, &waitError);
    if (!bundle || std::holds_alternative<QXmppError>(*bundle)) {
        if (error)
            *error = !bundle ? waitError : errorText(std::get<QXmppError>(*bundle));
        return false;
    }
    const auto publishedKey = std::get<XmppOmemoBundleItem>(*bundle).identityKey();
    if (publishedKey != own.keyId) {
        qWarning().noquote() << "Own OMEMO publication invalid: local-id=" << own.deviceId << "bundle-identity-key="
                             << (publishedKey.isEmpty() ? QStringLiteral("<missing>")
                                                        : QString::fromLatin1(publishedKey.toHex()));
        if (error)
            *error = publishedKey.isEmpty() ? QStringLiteral("The published OMEMO bundle has no identity key")
                                            : QStringLiteral("The published OMEMO identity key does not match");
        return false;
    }
    auto list = awaitTask(pubSub_->requestItem<XmppOmemoDeviceListItem>(QXmppUtils::jidToBareJid(config_.jid),
                                                                        QStringLiteral("urn:xmpp:omemo:2:devices"),
                                                                        QStringLiteral("current")),
                          config_.timeoutMs, &waitError);
    if (!list || std::holds_alternative<QXmppError>(*list)) {
        if (error)
            *error = !list ? waitError : errorText(std::get<QXmppError>(*list));
        return false;
    }
    const auto &devices = std::get<XmppOmemoDeviceListItem>(*list).devices();
    QStringList publishedIds;
    for (const auto &device : devices)
        publishedIds.append(device.id);
    const bool announced = std::any_of(devices.cbegin(), devices.cend(),
                                       [&own](const auto &device) { return device.id.toUInt() == own.deviceId; });
    qInfo().noquote() << "Own OMEMO publication check: local-id=" << own.deviceId
                      << "bundle-key-match=true published-device-ids=" << publishedIds.join(QLatin1Char(','))
                      << "announced=" << announced;
    if (!announced) {
        if (error)
            *error = QStringLiteral("The local OMEMO device is missing from the published device list");
        return false;
    }
    return true;
}

XmppStatusResult XmppWorker::repairOwnOmemoDevice()
{
    const auto connected = connectToServer();
    if (!connected.ok)
        return connected;
    const auto omemo = ensureOmemo();
    if (!omemo.ok)
        return omemo;
    const auto oldDeviceId  = omemoStorage_->ownDeviceId();
    const auto oldDeviceKey = omemoStorage_->ownIdentityKey();
    QString    error;
    auto       oldBundle = awaitTask(pubSub_->requestItem<XmppOmemoBundleItem>(QXmppUtils::jidToBareJid(config_.jid),
                                                                               QStringLiteral("urn:xmpp:omemo:2:bundles"),
                                                                               QString::number(oldDeviceId)),
                                     config_.timeoutMs, &error);
    const bool bundleIsValid = oldBundle && std::holds_alternative<XmppOmemoBundleItem>(*oldBundle)
        && std::get<XmppOmemoBundleItem>(*oldBundle).identityKey() == oldDeviceKey;
    if (bundleIsValid) {
        const auto bareJid = QXmppUtils::jidToBareJid(config_.jid);
        auto       list    = awaitTask(pubSub_->requestItem<XmppOmemoDeviceListItem>(
                                  bareJid, QStringLiteral("urn:xmpp:omemo:2:devices"), QStringLiteral("current")),
                                       config_.timeoutMs, &error);
        if (!list || std::holds_alternative<QXmppError>(*list))
            return { false, false, false, !list ? error : errorText(std::get<QXmppError>(*list)) };
        auto item    = std::get<XmppOmemoDeviceListItem>(*list);
        auto devices = item.devices();
        if (std::none_of(devices.cbegin(), devices.cend(),
                         [oldDeviceId](const auto &device) { return device.id.toUInt() == oldDeviceId; })) {
            devices.append({ config_.resource, QString::number(oldDeviceId), {} });
            item.setDevices(std::move(devices));
            auto published = awaitTask(pubSub_->publishItem(bareJid, QStringLiteral("urn:xmpp:omemo:2:devices"), item),
                                       config_.timeoutMs, &error);
            if (!published || std::holds_alternative<QXmppError>(*published))
                return { false, false, false, !published ? error : errorText(std::get<QXmppError>(*published)) };
        }
        return { true };
    }
    if (!awaitVoidTask(omemoManager_->resetOwnDeviceLocally(), config_.timeoutMs, &error))
        return { false, false, false, error };
    omemoReady_ = false;
    auto setup  = awaitTask(omemoManager_->setUp(config_.resource), config_.timeoutMs, &error);
    if (!setup)
        return { false, false, false, error };
    if (!*setup)
        return { false, false, false, QStringLiteral("QXmpp could not create a replacement OMEMO device") };
    omemoReady_ = true;

    const auto bareJid = QXmppUtils::jidToBareJid(config_.jid);
    auto       list    = awaitTask(pubSub_->requestItem<XmppOmemoDeviceListItem>(
                              bareJid, QStringLiteral("urn:xmpp:omemo:2:devices"), QStringLiteral("current")),
                                   config_.timeoutMs, &error);
    if (!list || std::holds_alternative<QXmppError>(*list))
        return { false, false, false, !list ? error : errorText(std::get<QXmppError>(*list)) };
    auto item    = std::get<XmppOmemoDeviceListItem>(*list);
    auto devices = item.devices();
    devices.removeIf([oldDeviceId](const auto &device) { return device.id.toUInt() == oldDeviceId; });
    const auto newDeviceId = omemoStorage_->ownDeviceId();
    if (std::none_of(devices.cbegin(), devices.cend(),
                     [newDeviceId](const auto &device) { return device.id.toUInt() == newDeviceId; }))
        devices.append({ config_.resource, QString::number(newDeviceId), {} });
    item.setDevices(std::move(devices));
    auto published = awaitTask(pubSub_->publishItem(bareJid, QStringLiteral("urn:xmpp:omemo:2:devices"), item),
                               config_.timeoutMs, &error);
    if (!published || std::holds_alternative<QXmppError>(*published))
        return { false, false, false, !published ? error : errorText(std::get<QXmppError>(*published)) };
    if (oldDeviceId) {
        auto retracted = awaitTask(
            pubSub_->retractItem(bareJid, QStringLiteral("urn:xmpp:omemo:2:bundles"), QString::number(oldDeviceId)),
            config_.timeoutMs, &error);
        if (!retracted)
            qWarning().noquote() << "Could not retract the old OMEMO bundle" << oldDeviceId << error;
    }
    return { true };
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
    keySyncExtension_->replyWithKey(requestId, SecureEnvelope::encodeRecoveryKey(config_.masterKey));
}

void XmppWorker::handleKeySyncRequest(const QString &requestId, const QString &from, const QByteArray &senderKey)
{
    if (QXmppUtils::jidToBareJid(from) != QXmppUtils::jidToBareJid(config_.jid)) {
        keySyncExtension_->reject(requestId);
        return;
    }

    QString waitError;
    auto    trust = awaitTask(omemoManager_->trustLevel(QXmppUtils::jidToBareJid(config_.jid), senderKey),
                              config_.timeoutMs, &waitError);
    if (!trust) {
        keySyncExtension_->reject(requestId);
        emit workerError(waitError);
        return;
    }
    if (*trust != QXmpp::TrustLevel::ManuallyTrusted && *trust != QXmpp::TrustLevel::Authenticated) {
        const auto devices   = ownOmemoDevices(&waitError);
        const bool ownDevice = std::any_of(devices.cbegin(), devices.cend(),
                                           [&](const XmppDeviceInfo &device) { return device.keyId == senderKey; });
        if (!ownDevice) {
            keySyncExtension_->reject(requestId);
            emit workerError(waitError.isEmpty()
                                 ? QStringLiteral("Ignored a storage-key request from an unknown OMEMO device")
                                 : waitError);
            return;
        }
        pendingInboundKeyRequests_.insert(requestId, { senderKey });
        emit keySyncTrustRequested(requestId, senderKey);
        return;
    }
    if (config_.masterKey.size() != SecureEnvelope::MasterKeySize) {
        keySyncExtension_->reject(requestId);
        return;
    }
    keySyncExtension_->replyWithKey(requestId, SecureEnvelope::encodeRecoveryKey(config_.masterKey));
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

XmppRekeyResult XmppWorker::rekeyStorage(const QList<QByteArray> &keys, const QByteArray &canonicalKey)
{
    XmppRekeyResult output;
    const auto      ready = ensureReady();
    if (!ready.ok) {
        output.error = ready.error;
        return output;
    }
    if (canonicalKey.size() != SecureEnvelope::MasterKeySize) {
        output.error = QStringLiteral("The selected canonical XMPP storage key is invalid");
        return output;
    }
    QHash<QByteArray, QByteArray> keyring;
    for (const auto &key : keys) {
        if (key.size() == SecureEnvelope::MasterKeySize)
            keyring.insert(SecureEnvelope::keyId(key), key);
    }
    keyring.insert(SecureEnvelope::keyId(canonicalKey), canonicalKey);

    QString waitError;
    auto idsResult = awaitTask(pubSub_->requestOwnPepItemIds(config_.indexNodeName()), config_.timeoutMs, &waitError);
    if (!idsResult || std::holds_alternative<QXmppError>(*idsResult)) {
        output.error = !idsResult ? waitError : errorText(std::get<QXmppError>(*idsResult));
        return output;
    }
    const auto ids = std::get<QVector<QString>>(*idsResult);
    output.total   = ids.size();
    for (const auto &id : ids) {
        auto indexResult = awaitTask(
            pubSub_->requestItem<QtNotePubSubItem>(QXmppUtils::jidToBareJid(config_.jid), config_.indexNodeName(), id),
            config_.timeoutMs, &waitError);
        auto contentResult = awaitTask(pubSub_->requestItem<QtNotePubSubItem>(QXmppUtils::jidToBareJid(config_.jid),
                                                                              config_.contentNodeName(), id),
                                       config_.timeoutMs, &waitError);
        if (!indexResult || !contentResult || std::holds_alternative<QXmppError>(*indexResult)
            || std::holds_alternative<QXmppError>(*contentResult)) {
            output.error = !indexResult || !contentResult
                ? waitError
                : errorText(std::holds_alternative<QXmppError>(*indexResult) ? std::get<QXmppError>(*indexResult)
                                                                             : std::get<QXmppError>(*contentResult));
            return output;
        }
        const auto &indexItem   = std::get<QtNotePubSubItem>(*indexResult);
        const auto &contentItem = std::get<QtNotePubSubItem>(*contentResult);
        if (!indexItem.isValid() || !contentItem.isValid()) {
            output.error = !indexItem.isValid() ? indexItem.parseError() : contentItem.parseError();
            return output;
        }
        const auto indexKey   = keyring.value(indexItem.payload().keyId);
        const auto contentKey = keyring.value(contentItem.payload().keyId);
        if (indexKey.isEmpty() || contentKey.isEmpty()) {
            output.inaccessibleNoteIds.append(id);
            continue;
        }
        auto note = XmppNoteCodec::decodeIndex(indexItem.payload(), indexKey, config_.indexNodeName());
        if (!note) {
            output.error = note.error.message;
            return output;
        }
        auto content
            = XmppNoteCodec::decodeContent(contentItem.payload(), contentKey, config_.contentNodeName(), note.value);
        if (!content) {
            output.error = content.error.message;
            return output;
        }
        note.value.content        = content.value;
        note.value.contentPresent = true;
        auto newContent           = XmppNoteCodec::encodeContent(note.value, canonicalKey, config_.contentNodeName());
        auto newIndex             = XmppNoteCodec::encodeIndex(note.value, canonicalKey, config_.indexNodeName());
        if (!newContent || !newIndex) {
            output.error = !newContent ? newContent.error.message : newIndex.error.message;
            return output;
        }
        auto publishedContent
            = awaitTask(pubSub_->publishOwnPepItem(config_.contentNodeName(), QtNotePubSubItem(newContent.value),
                                                   privatePublishOptions()),
                        config_.timeoutMs, &waitError);
        if (!publishedContent || std::holds_alternative<QXmppError>(*publishedContent)) {
            output.error = !publishedContent ? waitError : errorText(std::get<QXmppError>(*publishedContent));
            return output;
        }
        auto publishedIndex
            = awaitTask(pubSub_->publishOwnPepItem(config_.indexNodeName(), QtNotePubSubItem(newIndex.value),
                                                   privatePublishOptions()),
                        config_.timeoutMs, &waitError);
        if (!publishedIndex || std::holds_alternative<QXmppError>(*publishedIndex)) {
            output.error = !publishedIndex ? waitError : errorText(std::get<QXmppError>(*publishedIndex));
            return output;
        }
        ++output.migrated;
    }
    output.ok = output.inaccessibleNoteIds.isEmpty();
    if (!output.ok)
        output.error = QStringLiteral("Some notes use storage keys that are not available");
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
