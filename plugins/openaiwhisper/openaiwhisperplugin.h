#ifndef OPENAIWHISPERPLUGIN_H
#define OPENAIWHISPERPLUGIN_H

#include <QNetworkAccessManager>
#include <QPointer>

#include "pluginoptionsinterface.h"
#include "qtnoteplugininterface.h"
#include "speechrecognitionprovider.h"

class QDialog;

namespace QtNote {

class OpenAIWhisperPlugin : public QObject,
                            public PluginInterface,
                            public RegularPluginInterface,
                            public PluginOptionsTooltipInterface,
                            public PluginOptionsInterface,
                            public SpeechRecognitionProviderInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.openaiwhisper")
    Q_INTERFACES(QtNote::PluginInterface QtNote::RegularPluginInterface QtNote::PluginOptionsTooltipInterface
                     QtNote::PluginOptionsInterface QtNote::SpeechRecognitionProviderInterface)
public:
    explicit OpenAIWhisperPlugin(QObject *parent = nullptr);
    ~OpenAIWhisperPlugin() override;

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
    friend class OpenAIWhisperSpeechJob;
    friend class OpenAIWhisperSettingsDialog;

    struct Settings {
        QString apiKey;
        QString model;
        QString prompt;
    };

    Settings settings() const;
    void     saveSettings(const Settings &settings);
    void     addUsage(qint64 audioMs, qint64 bytesSent);

    QNetworkAccessManager *network = nullptr;
};

} // namespace QtNote

#endif // OPENAIWHISPERPLUGIN_H
