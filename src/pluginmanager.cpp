#include <QSettings>
#include <QStringList>
#include <QDir>
#include <QPluginLoader>

#include "pluginmanager.h"

class PluginsIterator
{
	QDir currentDir;
	QStringList pluginsDirs;
	QStringList plugins;
	QStringList::const_iterator pluginsDirsIt;
	QStringList::const_iterator pluginsIt;

public:
	PluginsIterator()
	{
#ifdef DEVEL
		QDir pluginsDir = QDir(qApp->applicationDirPath());
#if defined(Q_OS_WIN)
		if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
			pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
		if (pluginsDir.dirName() == "MacOS") {
			pluginsDir.cdUp();
			pluginsDir.cdUp();
			pluginsDir.cdUp();
		}
#endif
		if (pluginsDir.dirName() == "src") {
			pluginsDir.cdUp();
		}
		pluginsDir.cd("plugins");

		foreach (const QString &dirName, pluginsDir.entryList(QDir::Dirs)) {
			pluginsDirs << pluginsDir.absoluteFilePath(dirName);
		}
#else

# if defined(Q_OS_UNIX) && ! defined(Q_OS_MAC)
		pluginsDirs << LIBDIR "/" APPNAME;
		// TODO add ~/.local/share/QtNote/plugins
# elif defined(Q_OS_WIN)
		pluginsDirs << qApp->applicationDirPath() + "/plugins";
# endif
#endif
		pluginsDirsIt = pluginsDirs.constBegin();
		findDirWithPlugins();
	}

	void next()
	{
		if (pluginsIt != plugins.constEnd()) {
			pluginsIt++;
		}
		if (pluginsIt == plugins.constEnd() && pluginsDirsIt != pluginsDirs.constEnd()) {
			pluginsDirsIt++;
			findDirWithPlugins();
		}
	}

	inline bool isFinished() const { return pluginsIt == plugins.constEnd() &&
				pluginsDirsIt == pluginsDirs.constEnd(); }

	inline QString fileName() const
	{
		if (pluginsIt != plugins.constEnd()) {
			return currentDir.absoluteFilePath(*pluginsIt);
		}
	}

private:
	void findDirWithPlugins()
	{
		while (pluginsDirsIt != pluginsDirs.constEnd()) {
			currentDir = QDir(dirName);
			if (currentDir.isReadable()) {
				plugins = currentDir.entryList(QDir::Files);
				if (!plugins.isEmpty()) {
					pluginsIt = plugins.constBegin();
					return;
				}
			}
			pluginsDirsIt++;
		}
		pluginsIt = plugins.constEnd();
	}
};

PluginManager::PluginManager(QObject *parent) :
    QObject(parent)
{
	QSettings s;
	PluginsIterator it;
	while (!it.isFinished()) {
		QFileInfo fi(it.fileName());
		if (!fi.isFile() || !fi.isReadable()) {
			continue;
		}
		QString baseName = fi.baseName();
		if (s.contains("plugins."+baseName)) {
			// metadata already exists. read it.
		} else {
			QPluginLoader loader(it.fileName());
			QObject *plugin = loader.instance();
			if (plugin) {
				QtNotePlugin *qnp = qobject_cast<TrayIconInterface*>(plugin);
				if (!qnp) {
					qDebug("foreign Qt plugin %s. ignore it", qPrintable(baseName));
					continue;
				}
				s.setValue("plugins."+baseName, qnp->metadata());
				/*if (!pluginTrayIcon && (pluginTrayIcon = qobject_cast<TrayIconInterface*>(plugin))) {
					continue;
				}*/
			}
		}

		it.next();
	}
}
