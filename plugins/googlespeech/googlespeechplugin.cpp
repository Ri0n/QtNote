#include "googlespeechplugin.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QtPlugin>

#include "qtnote_config.h"

namespace QtNote {

static const QLatin1String SettingsGroup("googlespeech");
static const QLatin1String OAuthScope("https://www.googleapis.com/auth/cloud-platform");

static QString base64Url(const QByteArray &data)
{
    auto encoded = data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    return QString::fromLatin1(encoded);
}

static QString randomVerifier()
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    QString           ret;
    ret.reserve(64);
    for (int i = 0; i < 64; ++i) {
        ret.append(QLatin1Char(alphabet[QRandomGenerator::global()->bounded(int(sizeof(alphabet) - 1))]));
    }
    return ret;
}

static QString tokenError(QNetworkReply *reply, const QByteArray &body)
{
    auto doc = QJsonDocument::fromJson(body);
    if (doc.isObject()) {
        auto obj = doc.object();
        auto err = obj.value(QLatin1String("error_description")).toString();
        if (err.isEmpty()) {
            err = obj.value(QLatin1String("error")).toString();
        }
        if (!err.isEmpty()) {
            return err;
        }
    }
    return reply->errorString();
}

class GoogleSpeechJob : public SpeechRecognitionJob {
    Q_OBJECT
public:
    GoogleSpeechJob(GoogleSpeechPlugin *plugin, const SpeechRecognitionAudio &audio,
                    const SpeechRecognitionRequest &request) :
        SpeechRecognitionJob(plugin), plugin(plugin), audio(audio), request(request)
    {
        QMetaObject::invokeMethod(this, &GoogleSpeechJob::start, Qt::QueuedConnection);
    }

public slots:
    void cancel() override
    {
        cancelled = true;
        if (reply) {
            reply->abort();
        }
    }

private slots:
    void start()
    {
        auto c = plugin->credentials();
        if (c.accessToken.isEmpty() || c.accessTokenExpires <= QDateTime::currentDateTimeUtc().addSecs(60)) {
            refreshAccessToken(c);
            return;
        }
        recognize(c);
    }

private:
    void refreshAccessToken(const GoogleSpeechPlugin::Credentials &credentials)
    {
        if (credentials.clientId.isEmpty() || credentials.refreshToken.isEmpty()) {
            emit failed(tr("Google Speech is not authorized"));
            return;
        }

        QUrlQuery form;
        form.addQueryItem(QLatin1String("client_id"), credentials.clientId);
        if (!credentials.clientSecret.isEmpty()) {
            form.addQueryItem(QLatin1String("client_secret"), credentials.clientSecret);
        }
        form.addQueryItem(QLatin1String("refresh_token"), credentials.refreshToken);
        form.addQueryItem(QLatin1String("grant_type"), QLatin1String("refresh_token"));

        QNetworkRequest req(QUrl(QLatin1String("https://oauth2.googleapis.com/token")));
        req.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));
        reply = plugin->network->post(req, form.query(QUrl::FullyEncoded).toUtf8());
        connect(reply, &QNetworkReply::finished, this, [this, credentials]() {
            auto r    = reply;
            auto body = r->readAll();
            reply     = nullptr;
            r->deleteLater();
            if (cancelled) {
                return;
            }
            if (r->error() != QNetworkReply::NoError) {
                emit failed(tokenError(r, body));
                return;
            }

            auto doc             = QJsonDocument::fromJson(body);
            auto obj             = doc.object();
            auto c               = credentials;
            c.accessToken        = obj.value(QLatin1String("access_token")).toString();
            int expiresIn        = obj.value(QLatin1String("expires_in")).toInt(3600);
            c.accessTokenExpires = QDateTime::currentDateTimeUtc().addSecs(expiresIn);
            plugin->saveCredentials(c);
            recognize(c);
        });
    }

    void recognize(const GoogleSpeechPlugin::Credentials &credentials)
    {
        if (credentials.accessToken.isEmpty()) {
            emit failed(tr("Google Speech access token is empty"));
            return;
        }

        QJsonObject config;
        config.insert(QLatin1String("encoding"), QLatin1String("LINEAR16"));
        config.insert(QLatin1String("sampleRateHertz"), audio.sampleRate);
        config.insert(QLatin1String("audioChannelCount"), audio.channels);
        config.insert(QLatin1String("languageCode"),
                      request.language.isEmpty() ? QLatin1String("en-US") : request.language);
        config.insert(QLatin1String("enableAutomaticPunctuation"), true);
        config.insert(QLatin1String("maxAlternatives"), 1);

        QJsonObject audioObject;
        audioObject.insert(QLatin1String("content"), QString::fromLatin1(audio.data.toBase64()));

        QJsonObject root;
        root.insert(QLatin1String("config"), config);
        root.insert(QLatin1String("audio"), audioObject);
        auto payload = QJsonDocument(root).toJson(QJsonDocument::Compact);

        QNetworkRequest req(QUrl(QLatin1String("https://speech.googleapis.com/v1/speech:recognize")));
        req.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
        req.setRawHeader("Authorization", QByteArray("Bearer ") + credentials.accessToken.toUtf8());
        if (!credentials.quotaProjectId.isEmpty()) {
            req.setRawHeader("x-goog-user-project", credentials.quotaProjectId.toUtf8());
        }

        reply = plugin->network->post(req, payload);
        connect(reply, &QNetworkReply::finished, this, [this, payload]() {
            auto r    = reply;
            auto body = r->readAll();
            reply     = nullptr;
            r->deleteLater();
            if (cancelled) {
                return;
            }
            if (r->error() != QNetworkReply::NoError) {
                emit failed(tokenError(r, body));
                return;
            }

            auto        doc = QJsonDocument::fromJson(body);
            auto        obj = doc.object();
            QStringList parts;
            for (const auto &resultValue : obj.value(QLatin1String("results")).toArray()) {
                auto alternatives = resultValue.toObject().value(QLatin1String("alternatives")).toArray();
                if (!alternatives.isEmpty()) {
                    parts.append(alternatives.first().toObject().value(QLatin1String("transcript")).toString());
                }
            }
            plugin->addUsage(audio.durationMs, payload.size());
            emit finished(parts.join(QString()).trimmed());
        });
    }

    GoogleSpeechPlugin      *plugin;
    SpeechRecognitionAudio   audio;
    SpeechRecognitionRequest request;
    QPointer<QNetworkReply>  reply;
    bool                     cancelled = false;
};

class GoogleSpeechSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit GoogleSpeechSettingsDialog(GoogleSpeechPlugin *plugin) : QDialog(), plugin(plugin)
    {
        setAttribute(Qt::WA_DeleteOnClose);

        auto c = plugin->credentials();
        auto l = new QVBoxLayout(this);

        auto form      = new QFormLayout;
        clientId       = new QLineEdit(c.clientId, this);
        clientSecret   = new QLineEdit(c.clientSecret, this);
        quotaProjectId = new QLineEdit(c.quotaProjectId, this);
        clientSecret->setEchoMode(QLineEdit::Password);
        form->addRow(tr("OAuth client ID:"), clientId);
        form->addRow(tr("OAuth client secret:"), clientSecret);
        form->addRow(tr("Quota project ID:"), quotaProjectId);
        l->addLayout(form);

        status = new QLabel(this);
        status->setWordWrap(true);
        l->addWidget(status);

        auto authButton = new QPushButton(tr("Authorize in browser"), this);
        connect(authButton, &QPushButton::clicked, this, &GoogleSpeechSettingsDialog::authorize);
        l->addWidget(authButton);

        buttons          = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
        auto clearButton = buttons->addButton(tr("Forget tokens"), QDialogButtonBox::DestructiveRole);
        connect(clearButton, &QPushButton::clicked, this, [this]() {
            this->plugin->clearTokens();
            updateStatus();
        });
        connect(buttons, &QDialogButtonBox::accepted, this, &GoogleSpeechSettingsDialog::saveAndAccept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        l->addWidget(buttons);

        updateStatus();
    }

private slots:
    void authorize()
    {
        GoogleSpeechPlugin::Credentials c = plugin->credentials();
        c.clientId                        = clientId->text().trimmed();
        c.clientSecret                    = clientSecret->text().trimmed();
        c.quotaProjectId                  = quotaProjectId->text().trimmed();
        plugin->saveCredentials(c);

        if (c.clientId.isEmpty()) {
            status->setText(tr("OAuth client ID is required."));
            return;
        }

        verifier       = randomVerifier();
        state          = randomVerifier();
        auto challenge = base64Url(QCryptographicHash::hash(verifier.toLatin1(), QCryptographicHash::Sha256));

        if (server) {
            server->close();
            server->deleteLater();
        }
        server = new QTcpServer(this);
        if (!server->listen(QHostAddress::LocalHost, 0)) {
            status->setText(server->errorString());
            return;
        }
        connect(server, &QTcpServer::newConnection, this, &GoogleSpeechSettingsDialog::callbackReceived);

        redirectUri = QStringLiteral("http://127.0.0.1:%1/").arg(server->serverPort());

        QUrlQuery query;
        query.addQueryItem(QLatin1String("client_id"), c.clientId);
        query.addQueryItem(QLatin1String("redirect_uri"), redirectUri);
        query.addQueryItem(QLatin1String("response_type"), QLatin1String("code"));
        query.addQueryItem(QLatin1String("scope"), OAuthScope);
        query.addQueryItem(QLatin1String("access_type"), QLatin1String("offline"));
        query.addQueryItem(QLatin1String("prompt"), QLatin1String("consent"));
        query.addQueryItem(QLatin1String("code_challenge"), challenge);
        query.addQueryItem(QLatin1String("code_challenge_method"), QLatin1String("S256"));
        query.addQueryItem(QLatin1String("state"), state);

        QUrl url(QLatin1String("https://accounts.google.com/o/oauth2/v2/auth"));
        url.setQuery(query);
        QDesktopServices::openUrl(url);
        status->setText(tr("Waiting for browser authorization..."));
    }

    void callbackReceived()
    {
        auto socket = server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            auto      requestLine = QString::fromLatin1(socket->readLine()).trimmed();
            auto      path        = requestLine.section(QLatin1Char(' '), 1, 1);
            QUrl      url(QStringLiteral("http://127.0.0.1") + path);
            QUrlQuery query(url);
            auto      code     = query.queryItemValue(QLatin1String("code"));
            auto      gotState = query.queryItemValue(QLatin1String("state"));
            auto      error    = query.queryItemValue(QLatin1String("error"));

            QByteArray response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n";
            response += "<html><body>You can return to QtNote.</body></html>";
            socket->write(response);
            socket->disconnectFromHost();
            server->close();

            if (!error.isEmpty()) {
                status->setText(error);
                return;
            }
            if (code.isEmpty() || gotState != state) {
                status->setText(tr("Invalid authorization callback."));
                return;
            }
            exchangeCode(code);
        });
    }

    void exchangeCode(const QString &code)
    {
        auto c = plugin->credentials();

        QUrlQuery form;
        form.addQueryItem(QLatin1String("client_id"), c.clientId);
        if (!c.clientSecret.isEmpty()) {
            form.addQueryItem(QLatin1String("client_secret"), c.clientSecret);
        }
        form.addQueryItem(QLatin1String("code"), code);
        form.addQueryItem(QLatin1String("code_verifier"), verifier);
        form.addQueryItem(QLatin1String("grant_type"), QLatin1String("authorization_code"));
        form.addQueryItem(QLatin1String("redirect_uri"), redirectUri);

        QNetworkRequest req(QUrl(QLatin1String("https://oauth2.googleapis.com/token")));
        req.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));
        auto reply = plugin->network->post(req, form.query(QUrl::FullyEncoded).toUtf8());
        connect(reply, &QNetworkReply::finished, this, [this, reply, c]() mutable {
            auto body = reply->readAll();
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                status->setText(tokenError(reply, body));
                return;
            }

            auto obj       = QJsonDocument::fromJson(body).object();
            c.accessToken  = obj.value(QLatin1String("access_token")).toString();
            c.refreshToken = obj.value(QLatin1String("refresh_token")).toString(c.refreshToken);
            c.accessTokenExpires
                = QDateTime::currentDateTimeUtc().addSecs(obj.value(QLatin1String("expires_in")).toInt(3600));
            plugin->saveCredentials(c);
            updateStatus();
            emit accepted();
        });
    }

    void saveAndAccept()
    {
        auto c           = plugin->credentials();
        c.clientId       = clientId->text().trimmed();
        c.clientSecret   = clientSecret->text().trimmed();
        c.quotaProjectId = quotaProjectId->text().trimmed();
        plugin->saveCredentials(c);
        accept();
    }

private:
    void updateStatus()
    {
        auto c = plugin->credentials();
        status->setText(
            c.refreshToken.isEmpty()
                ? tr("Not authorized.")
                : tr("Authorized. Access token expires at %1.").arg(c.accessTokenExpires.toLocalTime().toString()));
    }

    GoogleSpeechPlugin *plugin;
    QLineEdit          *clientId       = nullptr;
    QLineEdit          *clientSecret   = nullptr;
    QLineEdit          *quotaProjectId = nullptr;
    QLabel             *status         = nullptr;
    QDialogButtonBox   *buttons        = nullptr;
    QTcpServer         *server         = nullptr;
    QString             verifier;
    QString             state;
    QString             redirectUri;
};

GoogleSpeechPlugin::GoogleSpeechPlugin(QObject *parent) : QObject(parent) { network = new QNetworkAccessManager(this); }

GoogleSpeechPlugin::~GoogleSpeechPlugin() = default;

int GoogleSpeechPlugin::metadataVersion() const { return MetadataVersion; }

PluginMetadata GoogleSpeechPlugin::metadata()
{
    PluginMetadata md;
    md.id          = QLatin1String("googlespeech");
    md.name        = tr("Google Speech");
    md.description = tr("Google Cloud Speech-to-Text backend");
    md.author      = QLatin1String("Rion");
    md.version     = 0x000100;
    md.minVersion  = 0x030200;
    md.maxVersion  = QTNOTE_VERSION;
    return md;
}

void GoogleSpeechPlugin::setHost(PluginHostInterface *host) { Q_UNUSED(host); }

bool GoogleSpeechPlugin::init(Main *qtnote)
{
    Q_UNUSED(qtnote);
    return true;
}

QString GoogleSpeechPlugin::tooltip() const
{
    auto        c = credentials();
    QStringList lines;
    lines.append(c.refreshToken.isEmpty() ? tr("<b>Speech:</b> not authorized") : tr("<b>Speech:</b> authorized"));
    auto stats = speechRecognitionUsageStats();
    if (stats.audioMsUsed > 0) {
        lines.append(tr("<b>Used:</b> %1 seconds, %2 bytes sent").arg(stats.audioMsUsed / 1000).arg(stats.bytesSent));
    }
    return lines.join(QLatin1String("<br/>"));
}

QDialog *GoogleSpeechPlugin::optionsDialog() { return new GoogleSpeechSettingsDialog(this); }

bool GoogleSpeechPlugin::isSpeechRecognitionReady() const
{
    auto c = credentials();
    return !c.clientId.isEmpty() && (!c.refreshToken.isEmpty() || !c.accessToken.isEmpty());
}

SpeechRecognitionCapabilities GoogleSpeechPlugin::speechRecognitionCapabilities() const
{
    SpeechRecognitionCapabilities caps;
    caps.supportsOneShot      = true;
    caps.supportsPunctuation  = true;
    caps.maxOneShotDurationMs = 60000;
    caps.languages = QStringList() << QStringLiteral("en-US") << QStringLiteral("en-GB") << QStringLiteral("ru-RU")
                                   << QStringLiteral("be-BY") << QStringLiteral("uk-UA") << QStringLiteral("de-DE")
                                   << QStringLiteral("fr-FR") << QStringLiteral("es-ES") << QStringLiteral("it-IT")
                                   << QStringLiteral("pl-PL") << QStringLiteral("pt-BR") << QStringLiteral("tr-TR");
    caps.preferredLanguage = QLocale::system().name().replace(QLatin1Char('_'), QLatin1Char('-'));
    return caps;
}

SpeechRecognitionUsageStats GoogleSpeechPlugin::speechRecognitionUsageStats() const
{
    QSettings s;
    s.beginGroup(SettingsGroup);
    SpeechRecognitionUsageStats stats;
    stats.available   = true;
    stats.audioMsUsed = s.value(QLatin1String("stats/audioMsUsed"), 0).toLongLong();
    stats.bytesSent   = s.value(QLatin1String("stats/bytesSent"), 0).toLongLong();
    return stats;
}

SpeechRecognitionJob *GoogleSpeechPlugin::recognizeSpeech(const SpeechRecognitionAudio   &audio,
                                                          const SpeechRecognitionRequest &request)
{
    return new GoogleSpeechJob(this, audio, request);
}

GoogleSpeechPlugin::Credentials GoogleSpeechPlugin::credentials() const
{
    QSettings s;
    s.beginGroup(SettingsGroup);
    Credentials c;
    c.clientId           = s.value(QLatin1String("clientId")).toString();
    c.clientSecret       = s.value(QLatin1String("clientSecret")).toString();
    c.refreshToken       = s.value(QLatin1String("refreshToken")).toString();
    c.accessToken        = s.value(QLatin1String("accessToken")).toString();
    c.accessTokenExpires = s.value(QLatin1String("accessTokenExpires")).toDateTime();
    c.quotaProjectId     = s.value(QLatin1String("quotaProjectId")).toString();
    return c;
}

void GoogleSpeechPlugin::saveCredentials(const Credentials &c)
{
    QSettings s;
    s.beginGroup(SettingsGroup);
    s.setValue(QLatin1String("clientId"), c.clientId);
    s.setValue(QLatin1String("clientSecret"), c.clientSecret);
    s.setValue(QLatin1String("refreshToken"), c.refreshToken);
    s.setValue(QLatin1String("accessToken"), c.accessToken);
    s.setValue(QLatin1String("accessTokenExpires"), c.accessTokenExpires);
    s.setValue(QLatin1String("quotaProjectId"), c.quotaProjectId);
}

void GoogleSpeechPlugin::clearTokens()
{
    auto c = credentials();
    c.refreshToken.clear();
    c.accessToken.clear();
    c.accessTokenExpires = QDateTime();
    saveCredentials(c);
}

void GoogleSpeechPlugin::addUsage(qint64 audioMs, qint64 bytesSent)
{
    QSettings s;
    s.beginGroup(SettingsGroup);
    s.setValue(QLatin1String("stats/audioMsUsed"),
               s.value(QLatin1String("stats/audioMsUsed"), 0).toLongLong() + audioMs);
    s.setValue(QLatin1String("stats/bytesSent"), s.value(QLatin1String("stats/bytesSent"), 0).toLongLong() + bytesSent);
}

} // namespace QtNote

#include "googlespeechplugin.moc"
