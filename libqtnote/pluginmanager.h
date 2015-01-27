#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QSharedPointer>
#include <QLibrary>

#include "../plugins/qtnoteplugininterface.h"

namespace QtNote {

class PluginManager : public QObject
{
	Q_OBJECT
public:
	enum LoadStatus {
		LS_Undefined,
		LS_NotPlugin,
		LS_Unloaded,
		LS_Loaded,
		LS_ErrVersion,
		LS_ErrAbi
	};

	enum LoadPolicy {
		LP_Auto,
		LP_Enabled,
		LP_Disabled
	};

	enum PluginFeature {
		DEIntegration   = 0x1,
		TrayIcon        = 0x2,
		GlobalShortcuts = 0x4,
		NoteStorage     = 0x8
	};
	Q_DECLARE_FLAGS(PluginFeatures, PluginFeature)

	class PluginData
	{
	public:
		typedef QSharedPointer<PluginData> Ptr;

		PluginData() : instance(0), loadStatus(LS_Undefined) {}
		QObject *instance;
		LoadPolicy loadPolicy;
		LoadStatus loadStatus;
		QString fileName;
		QDateTime modifyTime;
		PluginManager::PluginFeatures features;
		PluginMetadata metadata;
	};

	explicit PluginManager(Main *parent);

	void loadPlugins();
	LoadPolicy loadPolicy(const QString &pluginName) const { return plugins[pluginName]->loadPolicy; }
	LoadStatus loadStatus(const QString &pluginName) const { return plugins[pluginName]->loadStatus; }
	void setLoadPolicy(const QString &pluginName, LoadPolicy lp);
	inline int pluginsCount() const { return plugins.size(); }
	QStringList pluginsNames() const;
	inline QIcon icon(const QString &pluginName) const { return plugins[pluginName]->metadata.icon; }
	inline QString filename(const QString &pluginName) const { return plugins[pluginName]->fileName; }
	inline QString tooltip(const QString &pluginName) const {
		PluginData::Ptr pd = plugins[pluginName];
		PluginOptionsTooltipInterface *plugin;
		if ((pd->loadStatus == LS_Loaded) && (plugin = qobject_cast<PluginOptionsTooltipInterface*>(pd->instance))) {
			return plugin->tooltip();
		}
		return QString();
	}
signals:

public slots:

private:
	Main *qtnote;
	QHash<QString, PluginData::Ptr> plugins;

	LoadStatus loadPlugin(const QString &fileName, PluginData::Ptr &cache, QLibrary::LoadHints loadHints = 0);
	void updateMetadata();
};

}

#endif // PLUGINMANAGER_H
