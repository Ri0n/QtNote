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
		PluginMetadata metadata;
	};

	explicit PluginManager(Main *parent);

	void loadPlugins();
	LoadPolicy loadPolicy(const QString &pluginName) const { return plugins[pluginName]->loadPolicy; }
	void setLoadPolicy(const QString &pluginName, LoadPolicy lp);
	int pluginsCount() const { return plugins.size(); }
	QStringList pluginsNames() const;
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
