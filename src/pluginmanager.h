#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QHash>

class PluginManager : public QObject
{
	Q_OBJECT
public:

	struct Metadata
	{
		QString fileName;
		QString name;
		QString description;
		QString version;
		QString author;
	};

	explicit PluginManager(QObject *parent = 0);

signals:

public slots:

private:
	QHash<QString, Metadata> plugins;

};

#endif // PLUGINMANAGER_H
