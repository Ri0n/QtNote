#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>

#include "../plugins/qtnoteplugininterface.h"

class PluginManager : public QObject
{
	Q_OBJECT
public:

	explicit PluginManager(QObject *parent = 0);

signals:

public slots:

private:
	QHash<QString, PluginMetadata> plugins;

};

#endif // PLUGINMANAGER_H
