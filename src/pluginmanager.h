#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QDateTime>

#include "../plugins/qtnoteplugininterface.h"

class PluginManager : public QObject
{
	Q_OBJECT
public:

	struct PluginData
	{
		enum LoadStatus {
			LS_Undefined,
			LS_NotPlugin,
			LS_Unloaded,
			LS_Loaded,
			LS_ErrVersion,
			LS_ErrAbi
		};
		LoadStatus loadStatus;
		QString fileName;
		QDateTime modifyTime;
		PluginMetadata metadata;
	};

	explicit PluginManager(QObject *parent = 0);

signals:

public slots:

private:
	QHash<QString, PluginData> plugins;

};

#endif // PLUGINMANAGER_H
