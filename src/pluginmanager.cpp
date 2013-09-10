#include <QSettings>
#include <QStringList>
#include <QDir>
#include <QPluginLoader>
#include <QSet>

#include "pluginmanager.h"
#include "utils.h"
#include "qtnote.h"

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

PluginManager::PluginManager(QtNote *parent) :
	QObject(parent),
	qtnote(parent)
{
	QSettings s;

	s.beginGroup("plugins");
	foreach (const QString &pluginName, s.childGroups()) {
		s.beginGroup(pluginName);
		PluginData::Ptr pd(new PluginData);
		plugins[pluginName] = pd;
		pd->loadPolicy = (LoadPolicy)s.value("loadPolicy").toInt();
		pd->loadStatus = LS_Undefined;
		pd->fileName = s.value("filename").toString();
		pd->modifyTime = s.value("lastModified").toDateTime();
		pd->metadata.pluginType = (PluginMetadata::PluginType)s.value("pluginType").toInt();
		pd->metadata.name = pluginName;
		pd->metadata.description = s.value("description").toString();
		pd->metadata.author = s.value("author").toString();
		pd->metadata.version = s.value("version").toUInt();
		pd->metadata.minVersion = s.value("minVersion").toUInt();
		pd->metadata.maxVersion = s.value("maxVersion").toUInt();
		pd->metadata.extra = s.value("extra").toHash();
		//pd->metadata.extra = s.value("extra").();
		s.endGroup();
	}
	s.endGroup();
	updateMetadata();
}

void PluginManager::updateMetadata()
{
	QHash<QString, QString> file2name;
	foreach (const PluginData::Ptr &p, plugins) {
		file2name[p->fileName] = p->metadata.name;
	}

	PluginsIterator it;
	while (!it.isFinished()) {
		QString fileName = it.fileName();
		QString pluginName = file2name.value(fileName);
		PluginData::Ptr tmpCache;
		PluginData::Ptr &cache = pluginName.isEmpty()? tmpCache : plugins[pluginName];
		if (cache.isNull() || (cache->loadStatus != LS_Loaded &&
							   cache->modifyTime < QFileInfo(cache->fileName).lastModified())) { // have to update metadata cache

			loadPlugin(fileName, cache, QLibrary::ExportExternalSymbolsHint);
			if (!tmpCache.isNull()) { // new cache
				plugins.insert(tmpCache->metadata.name, tmpCache);
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
	QList<PluginData::Ptr> preferList;
	foreach (const PluginData::Ptr &pd, plugins) {
		if (pd->metadata.pluginType == PluginMetadata::DEIntegration &&
				pd->loadPolicy == LP_Enabled && (pd->loadStatus == LS_Undefined ||
												  pd->loadStatus == LS_Loaded)) {
			preferList.append(pd);
		}
	}
	foreach (const PluginData::Ptr &pd, plugins) {
		if (pd->metadata.pluginType == PluginMetadata::DEIntegration &&
				pd->loadPolicy != LP_Auto && (pd->loadStatus == LS_Undefined ||
												pd->loadStatus == LS_Loaded)) {
			preferList.append(pd);
		}
	}

	QMutableHashIterator<QString,PluginData::Ptr> mhi(plugins);
	while(mhi.hasNext()) {
		PluginData::Ptr &pd = mhi.next().value();
		if (pd->loadPolicy == LP_Enabled) {
			if (pd->loadStatus == LS_Loaded || (pd->loadStatus == LS_Undefined &&
												loadPlugin(pd->fileName, pd) == LS_Loaded)) {
				pd->instance->init(qtnote);
				return true;
			}
		} else { // auto
			QStringList de = pd->metadata.extra["de"].toStringList();
			QString session = qgetenv("DESKTOP_SESSION");
			if (de.contains(session) && (pd->loadStatus == LS_Loaded ||
										 loadPlugin(pd->fileName, pd) == LS_Loaded)) {
				pd->instance->init(qtnote);
				return true;
			}
		}
	}
	return false;
}

PluginManager::LoadStatus PluginManager::loadPlugin(const QString &fileName,
													PluginData::Ptr &cache, QLibrary::LoadHints loadHints)
{
	QPluginLoader loader(fileName);
	loader.setLoadHints(loadHints);
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

		if (!cache) {
			cache = PluginData::Ptr(new PluginData);
			cache->loadPolicy = md.pluginType ==  PluginMetadata::DEIntegration?
						LP_Auto : LP_Enabled;
		}
		if (cache->loadPolicy == LP_Disabled || loadHints & QLibrary::ExportExternalSymbolsHint) {
			loader.unload();
			loadStatus = LS_Unloaded; // actual status knows only QPluginLoader. probably I should fix it
		}
		cache->instance = loadStatus == LS_Loaded? qnp : 0;
		cache->fileName = fileName;
		cache->modifyTime = QFileInfo(fileName).lastModified();
		cache->loadStatus = loadStatus;
		cache->metadata = md;
		s.beginGroup(cache->metadata.name);
		s.setValue("loadPolicy", cache->loadPolicy);
		s.setValue("filename", cache->fileName);
		s.setValue("lastModify", cache->modifyTime);
		s.setValue("pluginType", md.pluginType);
		s.setValue("name", md.name);
		s.setValue("description", md.description);
		s.setValue("author", md.author);
		s.setValue("version", md.version);
		s.setValue("minVersion", md.minVersion);
		s.setValue("maxVersion", md.maxVersion);
		s.setValue("extra", md.extra);
		if (!cache->metadata.icon.isNull()) {
			cache->metadata.icon.pixmap(16, 16).save(Utils::localDataDir() + "/plugin-icons/" + md.name + ".png");
		}

		return loadStatus;
	}
	qDebug("failed to load %s : %s", qPrintable(fileName), qPrintable(loader.errorString()));
	return LS_NotPlugin;
}
