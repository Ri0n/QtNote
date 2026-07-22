#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QDateTime>
#include <QLibrary>
#include <QList>
#include <QObject>
#include <QSharedPointer>

#include "../plugins/pluginoptionsinterface.h"
#include "../plugins/qtnoteplugininterface.h"
#include "speechrecognitionprovider.h"

namespace QtNote {

class DesktopEditorPlatformBackend;
class PluginHost;

class PluginManager : public QObject {
    Q_OBJECT
public:
    enum LoadStatus {
        LS_Undefined = 0,
        LS_Loaded,
        LS_Initialized,
        LS_Errors       = 100,
        LS_ErrNotPlugin = LS_Errors + 1,
        LS_ErrVersion,
        LS_ErrAbi,
        LS_ErrMetadata,
        LS_Unloaded
    };

    enum LoadPolicy { LP_Auto, LP_Enabled, LP_Disabled };

    enum PluginFeature {
        FirstFeature    = 0x1,
        RegularPlugin   = FirstFeature,
        DEIntegration   = 0x2,
        TrayIcon        = 0x4,
        GlobalShortcuts = 0x8,
        Notifications   = 010,
        StickyNotes     = 0x10,
        LastFeature     = 0x20
    };
    Q_DECLARE_FLAGS(PluginFeatures, PluginFeature)

    class PluginData {
    public:
        typedef QSharedPointer<PluginData> Ptr;

        PluginData() : instance(0), loadPolicy(LP_Auto), loadPolicyExplicit(false), loadStatus(LS_Undefined) { }
        QObject                      *instance;
        LoadPolicy                    loadPolicy;
        bool                          loadPolicyExplicit;
        LoadStatus                    loadStatus;
        QString                       fileName;
        QDateTime                     modifyTime;
        PluginManager::PluginFeatures features;
        PluginMetadata                metadata;
    };

    explicit PluginManager(Main *parent);
    ~PluginManager();

    template <class T> T *castInterface(const PluginData::Ptr &pd) const
    {
        if (pd->loadPolicy != LP_Disabled && pd->loadStatus && pd->loadStatus < LS_Errors) {
            return qobject_cast<T *>(pd->instance);
        }
        return 0;
    }

    void       loadPlugins();
    LoadPolicy loadPolicy(const QString &pluginId) const { return plugins[pluginId]->loadPolicy; }
    LoadStatus loadStatus(const QString &pluginId) const { return plugins[pluginId]->loadStatus; }
    bool       isLoaded(const QString &pluginId) const
    {
        auto ls = plugins[pluginId]->loadStatus;
        return plugins[pluginId]->loadPolicy != LP_Disabled && ls && ls < LS_Errors;
    }
    void                  setLoadPolicy(const QString &pluginId, LoadPolicy lp);
    int                   pluginsCount() const { return plugins.size(); }
    QStringList           pluginsIds() const;
    const PluginMetadata &metadata(const QString &pluginId) const { return plugins[pluginId]->metadata; }
    QString               filename(const QString &pluginId) const { return plugins[pluginId]->fileName; }
    QString               tooltip(const QString &pluginId) const;
    bool                  canOptionsDialog(const QString &pluginId) const
    {
        return castInterface<PluginOptionsInterface>(plugins[pluginId]) != 0;
    }
    QDialog *optionsDialog(const QString &pluginId) const
    {
        auto plugin = castInterface<PluginOptionsInterface>(plugins[pluginId]);
        if (plugin) {
            return plugin->optionsDialog();
        }
        return 0;
    }
    QList<SpeechRecognitionProviderInterface *> speechRecognitionProviders() const;
    SpeechRecognitionProviderInterface         *speechRecognitionProvider() const;
    void                                        attachEditorPlatformBackend(DesktopEditorPlatformBackend *backend);

signals:

public slots:

private:
    Main                           *qtnote;
    PluginHost                     *pluginHost = nullptr;
    QHash<QString, PluginData::Ptr> plugins;

    LoadStatus loadPlugin(const QString &fileName, PluginData::Ptr &cache,
                          QLibrary::LoadHints loadHints = QLibrary::LoadHints());
    void       updateMetadata();
    bool       ensureLoaded(PluginData::Ptr pd);
    bool       initRegularPlugin(const PluginData::Ptr &pd);
    void       deinitRegularPlugin(const PluginData::Ptr &pd);
    QString    iconsCacheDir() const;
};

}

#endif // PLUGINMANAGER_H
