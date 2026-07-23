#include "mobilebundledplugins.h"

#include "bundledpluginregistry.h"
#include "geminiplugin.h"
#include "nextcloudplugin.h"
#include "openaiwhisperplugin.h"

namespace QtNote {

void registerMobileBundledPlugins(BundledPluginRegistry &registry)
{
    PluginListSource::Entry gemini;
    gemini.id           = QStringLiteral("gemini");
    gemini.name         = GeminiPlugin::tr("Gemini");
    gemini.description  = GeminiPlugin::tr("Gemini API speech recognition backend");
    gemini.versionText  = QStringLiteral("1.0");
    gemini.loadPolicy   = PluginListSource::LP_Auto;
    gemini.configurable = true;
    registry.registerFactory(gemini, [](QObject *parent) { return new GeminiPlugin(parent); });

    PluginListSource::Entry nextcloud;
    nextcloud.id          = QStringLiteral("nextcloud_storage");
    nextcloud.name        = NextcloudPlugin::tr("Nextcloud Notes");
    nextcloud.description = NextcloudPlugin::tr("Reads and writes notes using the Nextcloud Notes REST API");
    nextcloud.versionText = QStringLiteral("1.0");
    nextcloud.loadPolicy  = PluginListSource::LP_Auto;
    registry.registerFactory(nextcloud, [](QObject *parent) { return new NextcloudPlugin(parent); });

    PluginListSource::Entry openai;
    openai.id           = QStringLiteral("openaiwhisper");
    openai.name         = OpenAIWhisperPlugin::tr("OpenAI Whisper");
    openai.description  = OpenAIWhisperPlugin::tr("OpenAI Whisper speech recognition backend");
    openai.versionText  = QStringLiteral("1.0");
    openai.loadPolicy   = PluginListSource::LP_Disabled;
    openai.configurable = true;
    registry.registerFactory(openai, [](QObject *parent) { return new OpenAIWhisperPlugin(parent); });
}

} // namespace QtNote
