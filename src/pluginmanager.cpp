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

	s.beginGroup("plugins");
	foreach (const QString &pluginName, s.childGroups()) {
		s.beginGroup(pluginName);
		PluginData &pd = plugins[pluginName];
		pd.loadPolicy = (LoadPolicy)s.value("loadPolicy").toInt();
		pd.loadStatus = LS_Undefined;
		pd.fileName = s.value("filename").toString();
		pd.modifyTime = s.value("lastModified").toDateTime();
		pd.metadata.pluginType = (PluginMetadata::PluginType)s.value("pluginType").toInt();
		pd.metadata.name = pluginName;
		pd.metadata.description = s.value("description").toString();
		pd.metadata.author = s.value("author").toString();
		pd.metadata.version = s.value("version").toUInt();
		pd.metadata.minVersion = s.value("minVersion").toUInt();
		pd.metadata.maxVersion = s.value("maxVersion").toUInt();
		//pd.metadata.extra = s.value("extra").();
		s.endGroup();
	}
	s.endGroup();
}

void PluginManager::updateMetadata()
{
	QHash<QString, QString> file2name;
	QHash<QString, PluginData>::const_iterator cacheIt = plugins.begin();
	while (cacheIt != plugins.constEnd()) {
		file2name[cacheIt->fileName] = cacheIt->metadata.name;
		cacheIt++;
	}

	PluginsIterator it;
	while (!it.isFinished()) {
		QString fileName = it.fileName();
		QHash<QString, PluginData>::Iterator cacheIt = plugins.find(file2name[fileName]);
		if (cacheIt == plugins.end() || (cacheIt->loadStatus != LS_Loaded &&
										 cacheIt->modifyTime < QFileInfo(cacheIt->fileName).lastModified())) { // have to update metadata cache

			PluginData pd;
			pd.loadPolicy = cacheIt != plugins.end()? cacheIt->loadPolicy : LP_Enabled;
			LoadStatus ls = loadPlugin(fileName, &pd);
			if (ls != LS_Undefined) { // plugin data updated
				plugins.insert(pd.metadata.name, pd);
			}


			/*if (!pluginTrayIcon && (pluginTrayIcon = qobject_cast<TrayIconInterface*>(plugin))) {
				continue;
			}*/
		}

		it.next();
	}
}

bool PluginManager::loadDEIntegration()
{
	foreach (const PluginData &pd, plugins) {
		if (pd.loadPolicy == LP_Disabled || pd.metadata.pluginType != PluginMetadata::DEIntegration) {
			continue;
		}
		QStringList de = pd.metadata.extra["de"].toStringList();
		QString session = qgetenv("DESKTOP_SESSION");
		if (pd.loadPolicy == LP_Enabled || de.contains(session)) { // FIXME all plugins are LP_Enabled by default
			if (pd.loadStatus != LS_Loaded) {
				if (loadPlugin(pd.fileName) == LS_Loaded) {
					return true;
				}
			}
		}
	}
	return false;
}

PluginManager::LoadStatus PluginManager::loadPlugin(const QString &fileName, PluginData *pluginData)
{
	QPluginLoader loader(fileName);
	QSettings s;
	s.beginGroup("plugins");
	QObject *plugin = loader.instance();
	if (plugin) {
		QtNotePluginInterface *qnp = qobject_cast<QtNotePluginInterface *>(plugin);
		if (!qnp) {
			loader.unload();
			qDebug("foreign Qt plugin %s. ignore it", qPrintable(fileName));
			return LS_ErrAbi;
		}

		LoadStatus loadStatus = LS_Loaded;
		PluginMetadata md = qnp->metadata();
		if ((QTNOTE_VERSION  < md.minVersion) || (QTNOTE_VERSION  > md.maxVersion)) {
			loader.unload();
			qDebug("Incompatible version of qtnote plugin %s. ignore it", qPrintable(fileName));
			loadStatus = LS_ErrVersion;
		}

		if (pluginData) {
			PluginData  &pd = *pluginData;
			if (pd.loadPolicy == LP_Disabled) {
				loader.unload();
				loadStatus = LS_Unloaded; // actual status knows only QPluginLoader. probably I should fix it
			}
			pd.instance = loadStatus == LS_Loaded? qnp : 0;
			pd.fileName = fileName;
			pd.modifyTime = QFileInfo(fileName).lastModified();
			pd.loadStatus = loadStatus;
			pd.metadata = md;
			s.beginGroup(pd.metadata.name);
			s.setValue("loadPolicy", pd.loadPolicy);
			s.setValue("filename", pd.fileName);
			s.setValue("lastModify", pd.modifyTime);
			s.setValue("pluginType", md.pluginType);
			s.setValue("name", md.name);
			s.setValue("description", md.description);
			s.setValue("author", md.author);
			s.setValue("version", md.version);
			s.setValue("minVersion", md.minVersion);
			s.setValue("maxVersion", md.maxVersion);
			if (!pd.metadata.icon.isNull()) {
				pd.metadata.icon.pixmap(16, 16).save(Utils::localDataDir() + "/plugin-icons/" + md.name + ".png");
			}
		}

		return loadStatus;
	}
	qDebug("failed to load %s : %s", qPrintable(fileName), qPrintable(loader.errorString()));
	return LS_NotPlugin;
}
