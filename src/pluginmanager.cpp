#include <QSettings>
#include <QStringList>
#include <QDir>
#include <QPluginLoader>

#include "pluginmanager.h"
#include "utils.h"

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

		pluginsDirs << Utils::localDataDir() + "/plugins";
# if defined(Q_OS_UNIX) && ! defined(Q_OS_MAC)
		pluginsDirs << LIBDIR "/" APPNAME;		
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
		return QString();
	}

private:
	void findDirWithPlugins()
	{
		while (pluginsDirsIt != pluginsDirs.constEnd()) {
			currentDir = QDir(*pluginsDirsIt);
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

	s.beginGroup("plugins");
	foreach (const QString &pluginName, s.childGroups()) {
		s.beginGroup(pluginName);
		QFileInfo fi(s.value(QLatin1String("filename")).toString());
		if (fi.isFile() && fi.isReadable()) {
			PluginData &pd = plugins[fi.absoluteFilePath()];
			pd.loadStatus = PluginData::LS_Undefined;
			pd.fileName = fi.absoluteFilePath();
			pd.modifyTime = s.value("lastModified").toDateTime();
			pd.metadata.name = pluginName;
			pd.metadata.description = s.value("description").toString();
			pd.metadata.author = s.value("author").toString();
			pd.metadata.version = s.value("version").toUInt();
			pd.metadata.minVersion = s.value("minVersion").toUInt();
			pd.metadata.maxVersion = s.value("maxVersion").toUInt();
			//pd.metadata.extra = s.value("extra").();
			plugins.insert(pd.fileName, pd);
		} else {
			s.endGroup();
			s.remove(pluginName);
			continue;
		}
		s.endGroup();
	}

	QDir iconsDir = Utils::localDataDir();

	while (!it.isFinished()) {
		QFileInfo fi(it.fileName());
		if (!fi.isReadable()) {
			continue;
		}
		QHash<QString, PluginData>::Iterator cacheIt = plugins.find(fi.absoluteFilePath());
		if (cacheIt == plugins.end() || cacheIt->modifyTime < fi.lastModified()) { // have to update metadata cache

			QPluginLoader loader(cacheIt->fileName);
			QObject *plugin = loader.instance();
			if (plugin) {
				QtNotePluginInterface *qnp = qobject_cast<QtNotePluginInterface *>(plugin);
				if (!qnp) {
					loader.unload();
					if(cacheIt != plugins.end()) {
						cacheIt->loadStatus = PluginData::LS_ErrAbi;
					}
					qDebug("foreign Qt plugin %s. ignore it", qPrintable(fi.baseName()));
					continue;
				}

				PluginData  &pd = plugins[fi.absoluteFilePath()];
				pd.fileName = fi.absoluteFilePath();
				pd.modifyTime = fi.lastModified();
				pd.loadStatus = PluginData::LS_Loaded;
				pd.metadata = qnp->metadata();
				s.beginGroup(pd.metadata.name);
				s.setValue("filename", pd.fileName);
				s.setValue("lastModify", pd.modifyTime);
				s.setValue("name", pd.metadata.name);
				s.setValue("description", pd.metadata.description);
				s.setValue("author", pd.metadata.author);
				s.setValue("version", pd.metadata.version);
				s.setValue("minVersion", pd.metadata.minVersion);
				s.setValue("maxVersion", pd.metadata.maxVersion);

				if ((QTNOTE_VERSION  < pd.metadata.minVersion) || (QTNOTE_VERSION  > pd.metadata.maxVersion)) {
					cacheIt->loadStatus = PluginData::LS_ErrVersion;
					loader.unload();
					qDebug("Incompatible version of qtnote plugin %s. ignore it", qPrintable(fi.baseName()));
					continue;
				}
				/*if (!pluginTrayIcon && (pluginTrayIcon = qobject_cast<TrayIconInterface*>(plugin))) {
					continue;
				}*/
			} else if(cacheIt != plugins.end()) {
				cacheIt->loadStatus = PluginData::LS_NotPlugin;
			}
		}

		it.next();
	}
	s.endGroup();
}
