#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QDateTime>

#include "../plugins/qtnoteplugininterface.h"

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
		PluginData() : instance(0), loadStatus(LS_Undefined) {}
		QtNotePluginInterface *instance;
		LoadPolicy loadPolicy;
		LoadStatus loadStatus;
		QString fileName;
		QDateTime modifyTime;
		PluginMetadata metadata;
	};

	explicit PluginManager(QObject *parent = 0);

	bool loadDEIntegration();
signals:

public slots:

private:
	QHash<QString, PluginData> plugins;

	LoadStatus loadPlugin(const QString &fileName, PluginData *pluginData = 0);
	void updateMetadata();
};

#endif // PLUGINMANAGER_H
