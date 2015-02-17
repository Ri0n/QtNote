#include <QSettings>
#include <QStringList>
#include <QDir>
#include <QPluginLoader>
#include <QSet>
#include <QApplication>
#include <QDebug>

#include "pluginmanager.h"
#include "utils.h"
#include "qtnote.h"
#include "shortcutsmanager.h"

#include "deintegrationinterface.h"
#include "trayinterface.h"
#include "globalshortcutsinterface.h"

namespace QtNote {

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
		qDebug() << "Plugins dirs: " << pluginsDirs;
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
				plugins.clear();
				foreach(const QString &file, currentDir.entryList(QDir::Files)) {
					if (QLibrary::isLibrary(file)) {
						plugins.append(file);
					}
				}
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

PluginManager::PluginManager(Main *parent) :
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
        pd->modifyTime = QDateTime::fromTime_t(s.value("lastModify").toUInt()); // if 0 then we have staled cache. it's ok
		pd->features = (PluginFeatures)s.value("features").toUInt();
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

void PluginManager::loadPlugins()
{
	struct FeaturedPlugin {
		QStringList native;
		QStringList base;
	};
	
	QMap<PluginFeature,FeaturedPlugin> featurePriority;
    QList<PluginData::Ptr> regularPlugins;

	QSettings s;
	QStringList prioritizedList = s.value("plugins-priority").toStringList();
	QMutableStringListIterator it(prioritizedList);
	while (it.hasNext()) {
		if (!plugins.contains(it.next())) {
			it.remove();
		}
	}

	prioritizedList += (plugins.keys().toSet() - prioritizedList.toSet()).toList();
	s.setValue("plugins-priority", prioritizedList);

	/*
	 * now we have fully prioritized list.
	 * time to load the plugins. start from required unique features
	 * like tray integration, global shortcuts integrtion etc.
	 */
	QString session = qgetenv("DESKTOP_SESSION"); // FIXME prefer XDG_CURRENT_DESKTOP or QGenericUnixServices::desktopEnvironment for qt5
	foreach (const QString &plugin, prioritizedList) {
		PluginData::Ptr pd = plugins[plugin];
		QStringList deList = pd->metadata.extra["de"].toStringList();
		bool native = deList.contains(session);

        if (pd->loadPolicy == LP_Disabled || (!native && !deList.isEmpty())) {
			continue;
		}

		QList<PluginFeature> featureList;
		if (pd->features & TrayIcon) {
			featureList.append(TrayIcon);
		}
		if (pd->features & DEIntegration) {
			featureList.append(DEIntegration);
		}
		if (pd->features & GlobalShortcuts) {
			featureList.append(GlobalShortcuts);
		}
		foreach (PluginFeature f, featureList) {
			FeaturedPlugin &fp = featurePriority[f];
			if (native) {
				fp.native.push_back(plugin);
			} else {
				fp.base.push_back(plugin);
			}
		}

        if (pd->features & RegularPlugin) {
            regularPlugins.append(pd);
        }
	}

	/* set most desirable tray implementation */
	QStringList trayPlugins = featurePriority[TrayIcon].native + featurePriority[TrayIcon].base;
	foreach(const QString &plugin, trayPlugins) {
        auto pd = plugins[plugin];
        if (!ensureLoaded(pd)) {
            continue;
        }
        TrayInterface *tp = qobject_cast<TrayInterface *>(pd->instance);
		TrayImpl *tray = tp->initTray(qtnote);
		if (tray) {
			qtnote->setTrayImpl(tray);
            pd->loadStatus = LS_Initialized;
			break;
		}
	}

	/* set most desirable desktop environment plugin */
	QStringList dePlugins = featurePriority[DEIntegration].native + featurePriority[DEIntegration].base;
    foreach(const QString &plugin, dePlugins) {
        auto pd = plugins[plugin];
        if (!ensureLoaded(pd)) {
            continue;
        }
        qtnote->setDesktopImpl(qobject_cast<DEIntegrationInterface *>(pd->instance));
        pd->loadStatus = LS_Initialized;
        break;
	}

	/* set most desirable global shortcuts plugin */
	QStringList gsPlugins = featurePriority[GlobalShortcuts].native + featurePriority[GlobalShortcuts].base;
    foreach(const QString &plugin, gsPlugins) {
        auto pd = plugins[plugin];
        if (!ensureLoaded(pd)) {
            continue;
        }
        qtnote->setGlobalShortcutsImpl(qobject_cast<GlobalShortcutsInterface *>(pd->instance));
        pd->loadStatus = LS_Initialized;
        break;
	}

    foreach (PluginData::Ptr pd, regularPlugins) {
        if (!ensureLoaded(pd)) {
            continue;
        }
        if (qobject_cast<RegularPluginInterface*>(pd->instance)->init(qtnote)) {
            pd->loadStatus = LS_Initialized;
        }
    }
}

bool PluginManager::ensureLoaded(PluginData::Ptr pd)
{
    if (pd->loadStatus == LS_Undefined || pd->loadStatus == LS_Unloaded) {
        return loadPlugin(pd->fileName, pd) == LS_Loaded;
    }
    if (pd->loadStatus > LS_Errors) {
        return false;
    }
    return true;
}

void PluginManager::setLoadPolicy(const QString &pluginName, PluginManager::LoadPolicy lp)
{
	QSettings s;
	plugins[pluginName]->loadPolicy = lp;
	s.beginGroup("plugins");
	s.beginGroup(pluginName);
	s.setValue("loadPolicy", (int)lp);
}

QStringList PluginManager::pluginsNames() const
{
	return QSettings().value("plugins-priority").toStringList();
}

QString PluginManager::tooltip(const QString &pluginName) const {
    PluginData::Ptr pd = plugins[pluginName];
    PluginOptionsTooltipInterface *plugin;
    if ((pd->loadStatus && pd->loadStatus < LS_Errors) &&
            (plugin = qobject_cast<PluginOptionsTooltipInterface*>(pd->instance)))
    {
        return plugin->tooltip();
    }
    return QString();
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
        if (cache.isNull() || (cache->loadStatus == LS_Undefined &&
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

PluginManager::LoadStatus PluginManager::loadPlugin(const QString &fileName,
													PluginData::Ptr &cache, QLibrary::LoadHints loadHints)
{
#ifdef DEVEL
	qDebug("Loading plugin: %s", qPrintable(fileName));
#endif
	QPluginLoader loader(fileName);
	loader.setLoadHints(loadHints);
	QSettings s;
	s.beginGroup("plugins");
	QObject *plugin = loader.instance();
	if (plugin) {
		PluginInterface *qnp = qobject_cast<PluginInterface *>(plugin);
        if (!qnp || qnp->metadataVersion() != MetadataVerion) {
			loader.unload();
			qDebug("not QtNote plugin %s. ignore it", qPrintable(fileName));
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
			cache->loadPolicy = LP_Auto;
		}
		cache->instance = loadStatus == LS_Loaded? plugin : 0;
		cache->fileName = fileName;
		cache->modifyTime = QFileInfo(fileName).lastModified();
		cache->metadata = md;
		cache->features &= 0;
		if (qobject_cast<TrayInterface*>(plugin)) {
			cache->features |= TrayIcon;
		}
		if (qobject_cast<DEIntegrationInterface*>(plugin)) {
			cache->features |= DEIntegration;
		}
		if (qobject_cast<GlobalShortcutsInterface*>(plugin)) {
			cache->features |= GlobalShortcuts;
		}
        if (qobject_cast<RegularPluginInterface*>(plugin)) {
            cache->features |= RegularPlugin;
        }
		s.beginGroup(cache->metadata.name);
		s.setValue("loadPolicy", cache->loadPolicy);
		s.setValue("filename", cache->fileName);
        s.setValue("lastModify", cache->modifyTime.toTime_t());
		s.setValue("features", (uint)cache->features);
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
		if (cache->loadPolicy == LP_Disabled || loadHints & QLibrary::ExportExternalSymbolsHint) {
			loader.unload();
			loadStatus = LS_Unloaded; // actual status knows only QPluginLoader. probably I should fix it
		}
		cache->loadStatus = loadStatus;

		return loadStatus;
	}
	qDebug("failed to load %s : %s", qPrintable(fileName), qPrintable(loader.errorString()));
	return LS_NotPlugin;
}

} // namespace QtNote
