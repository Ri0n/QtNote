#include "xmppworker.h"

#include "qtnotepubsubitem.h"
#include "xmpppepextension.h"

#include <QEventLoop>
#include <QTimer>
#include <QUuid>
#include <QXmppClient.h>
#include <QXmppConfiguration.h>
#include <QXmppDiscoveryIq.h>
#include <QXmppDiscoveryManager.h>
#include <QXmppError.h>
#include <QXmppGlobal.h>
#include <QXmppPubSubManager.h>
#include <QXmppPubSubNodeConfig.h>
#include <QXmppPubSubPublishOptions.h>
#include <QXmppStanza.h>
#include <QXmppTask.h>
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
            && left.originId == right.originId && left.timeoutMs == right.timeoutMs;
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
    prepared_     = false;
    discovery_    = nullptr;
    pubSub_       = nullptr;
    pepExtension_ = nullptr;

    if (client_) {
        client_->disconnectFromServer();
        delete client_;
        client_ = nullptr;
    }
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
    pepExtension_->setOwnBareJid(config_.jid);
    pepExtension_->setNodeName(config_.nodeName);

    connect(pepExtension_, &XmppPepExtension::notePublished, this, &XmppWorker::remoteNotePublished);
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

XmppStatusResult XmppWorker::ensureNode()
{
    const auto verifyNode = [this]() -> XmppStatusResult {
        QString waitError;
        auto    verify
            = awaitTask(pubSub_->requestOwnPepNodeConfiguration(config_.nodeName), config_.timeoutMs, &waitError);
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
    auto request = awaitTask(pubSub_->requestOwnPepNodeConfiguration(config_.nodeName), config_.timeoutMs, &waitError);
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

        auto create = awaitTask(pubSub_->createOwnPepNode(config_.nodeName, privateNodeConfig()), config_.timeoutMs,
                                &waitError);
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

    auto configured = awaitTask(pubSub_->configureOwnPepNode(config_.nodeName, config), config_.timeoutMs, &waitError);
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
    if (prepared_) {
        return { true };
    }

    auto support = verifyPrivateStorageSupport();
    if (!support.ok) {
        return support;
    }

    auto node = ensureNode();
    if (!node.ok) {
        return node;
    }

    prepared_ = true;
    return { true };
}

XmppStatusResult XmppWorker::probe() { return ensureReady(); }

XmppListResult XmppWorker::listNotes()
{
    XmppListResult output;
    const auto     ready = ensureReady();
    if (!ready.ok) {
        output.error = ready.error;
        return output;
    }

    QString waitError;
    auto    idsResult = awaitTask(pubSub_->requestOwnPepItemIds(config_.nodeName), config_.timeoutMs, &waitError);

    if (idsResult && std::holds_alternative<QVector<QString>>(*idsResult)) {
        const auto    ids       = std::get<QVector<QString>>(*idsResult);
        constexpr int BatchSize = 50;

        for (int offset = 0; offset < ids.size(); offset += BatchSize) {
            QStringList batch;
            const int   end = qMin(offset + BatchSize, ids.size());
            for (int i = offset; i < end; ++i) {
                batch.append(ids.at(i));
            }

            auto itemsResult = awaitTask(
                pubSub_->requestItems<QtNotePubSubItem>(QXmppUtils::jidToBareJid(config_.jid), config_.nodeName, batch),
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
                output.notes.append(item.note());
            }
        }

        output.ok = true;
        return output;
    }

    // Compatibility fallback for servers that do not expose item IDs through disco#items.
    auto itemsResult
        = awaitTask(pubSub_->requestItems<QtNotePubSubItem>(QXmppUtils::jidToBareJid(config_.jid), config_.nodeName),
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
        output.notes.append(item.note());
    }
    output.ok = true;
    return output;
}

XmppNoteResult XmppWorker::requestNote(const QString &id)
{
    XmppNoteResult output;
    QString        waitError;
    auto           result
        = awaitTask(pubSub_->requestItem<QtNotePubSubItem>(QXmppUtils::jidToBareJid(config_.jid), config_.nodeName, id),
                    config_.timeoutMs, &waitError);
    if (!result) {
        output.error = waitError;
        return output;
    }
    if (const auto *error = std::get_if<QXmppError>(&*result)) {
        output.error    = errorText(*error);
        output.notFound = isItemNotFound(*error);
        return output;
    }

    const auto &item = std::get<QtNotePubSubItem>(*result);
    if (!item.isValid()) {
        output.error = item.parseError();
        return output;
    }
    output.note = item.note();
    output.ok   = true;
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

    QtNotePubSubItem item(updated);
    QString          waitError;
    auto             publish = awaitTask(pubSub_->publishOwnPepItem(config_.nodeName, item, privatePublishOptions()),
                                         config_.timeoutMs, &waitError);
    if (!publish) {
        output.error = waitError;
        return output;
    }
    if (const auto *error = std::get_if<QXmppError>(&*publish)) {
        output.error = errorText(*error);
        return output;
    }

    const QString returnedId = std::get<QString>(*publish);
    if (!returnedId.isEmpty()) {
        updated.id = returnedId;
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
    auto    result = awaitTask(pubSub_->retractItem(QXmppUtils::jidToBareJid(config_.jid), config_.nodeName, id, true),
                               config_.timeoutMs, &waitError);
    if (!result) {
        return { false, false, false, waitError };
    }
    if (const auto *error = resultError(*result)) {
        const auto text     = errorText(*error);
        const bool notFound = isItemNotFound(*error);
        return { notFound, false, notFound, notFound ? QString() : text };
    }
    return { true };
}

QString XmppWorker::newUuid() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }

QString XmppWorker::errorText(const QXmppError &error)
{
    return error.description.isEmpty() ? QStringLiteral("Unknown XMPP error") : error.description;
}

} // namespace QtNote
