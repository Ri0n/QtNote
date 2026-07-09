#ifndef GOOGLESPEECHPLUGIN_H
#define GOOGLESPEECHPLUGIN_H

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QSettings>

#include "pluginoptionsinterface.h"
#include "qtnoteplugininterface.h"
#include "speechrecognitionprovider.h"

class QDialog;
class QLabel;
class QLineEdit;
class QTcpServer;

namespace QtNote {

class GoogleSpeechPlugin : public QObject,
                           public PluginInterface,
                           public RegularPluginInterface,
                           public PluginOptionsTooltipInterface,
                           public PluginOptionsInterface,
                           public SpeechRecognitionProviderInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.googlespeech")
    Q_INTERFACES(QtNote::PluginInterface QtNote::RegularPluginInterface QtNote::PluginOptionsTooltipInterface
                     QtNote::PluginOptionsInterface QtNote::SpeechRecognitionProviderInterface)
public:
    explicit GoogleSpeechPlugin(QObject *parent = nullptr);
    ~GoogleSpeechPlugin() override;

    int            metadataVersion() const override;
    PluginMetadata metadata() override;
    void           setHost(PluginHostInterface *host) override;

    bool init(Main *qtnote) override;

    QString  tooltip() const override;
    QDialog *optionsDialog() override;

    bool                          isSpeechRecognitionReady() const override;
    SpeechRecognitionCapabilities speechRecognitionCapabilities() const override;
    SpeechRecognitionUsageStats   speechRecognitionUsageStats() const override;
    SpeechRecognitionJob         *recognizeSpeech(const SpeechRecognitionAudio   &audio,
                                                  const SpeechRecognitionRequest &request) override;

private:
    friend class GoogleSpeechJob;
    friend class GoogleSpeechSettingsDialog;

    struct Credentials {
        QString   clientId;
        QString   clientSecret;
        QString   refreshToken;
        QString   accessToken;
        QDateTime accessTokenExpires;
        QString   quotaProjectId;
    };

    Credentials credentials() const;
    void        saveCredentials(const Credentials &credentials);
    void        clearTokens();

    void addUsage(qint64 audioMs, qint64 bytesSent);

    QNetworkAccessManager *network = nullptr;
};

} // namespace QtNote

#endif // GOOGLESPEECHPLUGIN_H
