/*
QtNote - Simple note-taking application
Copyright (C) 2020 Sergei Ilinykh

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Contacts:
E-Mail: rion4ik@gmail.com XMPP: rion@jabber.ru
*/

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QPluginLoader>
#include <QSet>
#include <QSettings>
#include <QStringList>

#include "notewidget.h"
#include "pluginhost.h"
#include "pluginmanager.h"
#include "qtnote.h"
#include "shortcutsmanager.h"
#include "utils.h"

#include "deintegrationinterface.h"
#include "globalshortcutsinterface.h"
#include "notificationinterface.h"
#include "qtnote_config.h"
#include "trayinterface.h"

namespace QtNote {

class PluginsIterator {
    QDir                        currentDir;
    QStringList                 pluginsDirs;
    QStringList                 plugins;
    QStringList::const_iterator pluginsDirsIt;
    QStringList::const_iterator pluginsIt;

public:
    PluginsIterator()
    {
#ifdef QTNOTE_DEVEL
        QDir pluginsDir = QDir(qApp->applicationDirPath());

#  if defined(Q_OS_MAC)
        if (pluginsDir.dirName() == "MacOS") {
            pluginsDir.cdUp();
            pluginsDir.cdUp();
            pluginsDir.cdUp();
        }
#  else
        pluginsDir.cdUp(); // on linux application dir is <builddir>/src. probably same same on windows
#  endif
        pluginsDir.cd("plugins");
#  ifdef Q_OS_WIN
        pluginsDirs << pluginsDir.path();
        pluginsDir = QDir::current();
        pluginsDir.cd("plugins");
        pluginsDirs << pluginsDir.path();
#  else
        foreach (const QString &dirName, pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QDir d(pluginsDir);
            d.cd(dirName);
            pluginsDirs << d.path();
        }
#  endif
        qDebug() << "Plugins dirs: " << pluginsDirs;
#else // not devel

        pluginsDirs << Utils::qtnoteDataDir() + "/plugins";
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        pluginsDirs << LIBDIR "/" APPNAME;
#elif defined(Q_OS_WIN)
        pluginsDirs << qApp->applicationDirPath() + "/plugins";
#endif
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

    inline bool isFinished() const
    {
        return pluginsIt == plugins.constEnd() && pluginsDirsIt == pluginsDirs.constEnd();
    }

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
                foreach (const QString &file, currentDir.entryList(QDir::Files)) {
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

PluginManager::PluginManager(Main *parent) : QObject(parent), qtnote(parent), pluginHost(new PluginHost(this))
{
    QSettings s;

    QDir(iconsCacheDir()).mkpath(QLatin1String("."));
    updateMetadata();

    connect(qtnote, &Main::noteWidgetCreated, this, [this](QWidget *w) {
        NoteWidget *nw = qobject_cast<NoteWidget *>(w);
        connect(pluginHost, &PluginHost::rehightlight_requested, nw, [nw]() { nw->rehighlight(); });
    });
}

PluginManager::~PluginManager()
{
    foreach (const QString &pluginId, plugins.keys()) {
        if (isLoaded(pluginId)) {
            auto p = plugins[pluginId];
            delete p->instance;
            p->instance = 0;
        }
    }
}

void PluginManager::loadPlugins()
{
    struct FeaturedPlugin {
        QStringList native;
        QStringList base;
    };

    QMap<PluginFeature, FeaturedPlugin> featurePriority;
    QList<PluginData::Ptr>              regularPlugins;

    QSettings                  s;
    QStringList                prioritizedList = s.value("plugins-priority").toStringList();
    QMutableStringListIterator it(prioritizedList);
    while (it.hasNext()) {
        if (!plugins.contains(it.next())) {
            it.remove();
        }
    }

    auto pluginKeys     = QSet<QString>(plugins.keyBegin(), plugins.keyEnd());
    auto prioritizedSet = pluginKeys - QSet<QString>(prioritizedList.constBegin(), prioritizedList.constEnd());
    prioritizedList += QStringList(prioritizedSet.constBegin(), prioritizedSet.constEnd());
    s.setValue("plugins-priority", prioritizedList);

    /*
     * now we have fully prioritized list.
     * time to load the plugins. start from required unique features
     * like tray integration, global shortcuts integrtion etc.
     */
#ifdef Q_OS_OSX
    QString session = "macosx";
#else
    auto    desktopComponents = QString(qgetenv("XDG_CURRENT_DESKTOP")).split(":");
    QString session           = desktopComponents.isEmpty() ? QString {} : desktopComponents.last();
#endif
    foreach (const QString &plugin, prioritizedList) {
        PluginData::Ptr pd     = plugins[plugin];
        QStringList     deList = pd->metadata.extra["de"].toStringList();
        bool            native = deList.contains(session);

        if (pd->loadPolicy == LP_Disabled || (!native && !deList.isEmpty())) {
            continue;
        }

        auto f = FirstFeature;
        while (f != LastFeature) {
            if (pd->features & f) {
                FeaturedPlugin &fp = featurePriority[f];
                if (native) {
                    fp.native.push_back(plugin);
                } else {
                    fp.base.push_back(plugin);
                }
            }
            f = PluginFeature(f << 1);
        }

        if (pd->features & RegularPlugin) {
            regularPlugins.append(pd);
        }
    }

    /* set most desirable tray implementation */
    QStringList trayPlugins = featurePriority[TrayIcon].native + featurePriority[TrayIcon].base;
    foreach (const QString &plugin, trayPlugins) {
        auto pd = plugins[plugin];
        if (!ensureLoaded(pd)) {
            continue;
        }
        TrayInterface *tp   = qobject_cast<TrayInterface *>(pd->instance);
        TrayImpl      *tray = tp->initTray(qtnote);
        if (tray) {
            qtnote->setTrayImpl(tray);
            pd->loadStatus = LS_Initialized;
            break;
        }
    }

    /* set most desirable desktop environment plugin */
    QStringList dePlugins = featurePriority[DEIntegration].native + featurePriority[DEIntegration].base;
    foreach (const QString &plugin, dePlugins) {
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
    foreach (const QString &plugin, gsPlugins) {
        auto pd = plugins[plugin];
        if (!ensureLoaded(pd)) {
            continue;
        }
        qtnote->setGlobalShortcutsImpl(qobject_cast<GlobalShortcutsInterface *>(pd->instance));
        pd->loadStatus = LS_Initialized;
        break;
    }

    /* set most desirable notifications plugin */
    QStringList nPlugins = featurePriority[Notifications].native + featurePriority[Notifications].base;
    foreach (const QString &plugin, nPlugins) {
        auto pd = plugins[plugin];
        if (!ensureLoaded(pd)) {
            continue;
        }
        qtnote->setNotificationImpl(qobject_cast<NotificationInterface *>(pd->instance));
        pd->loadStatus = LS_Initialized;
        break;
    }

    foreach (PluginData::Ptr pd, regularPlugins) {
        if (!ensureLoaded(pd)) {
            continue;
        }
        if (qobject_cast<RegularPluginInterface *>(pd->instance)->init(qtnote)) {
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

QString PluginManager::iconsCacheDir() const { return Utils::qtnoteDataDir() + "/plugin-icons/"; }

void PluginManager::setLoadPolicy(const QString &pluginId, PluginManager::LoadPolicy lp)
{
    QSettings s;
    plugins[pluginId]->loadPolicy = lp;
    s.beginGroup("plugins");
    s.beginGroup(pluginId);
    s.setValue("loadPolicy", (int)lp);
}

QStringList PluginManager::pluginsIds() const { return QSettings().value("plugins-priority").toStringList(); }

QString PluginManager::tooltip(const QString &pluginId) const
{
    PluginData::Ptr                pd = plugins[pluginId];
    PluginOptionsTooltipInterface *plugin;
    if ((pd->loadStatus && pd->loadStatus < LS_Errors)
        && (plugin = qobject_cast<PluginOptionsTooltipInterface *>(pd->instance))) {
        return plugin->tooltip();
    }
    return QString();
}

void PluginManager::updateMetadata()
{
    QSettings                       s;
    decltype(plugins)               tmpPlugins;
    QHash<QString, PluginData::Ptr> file2data;

    s.beginGroup("plugins");
    foreach (const QString &pluginId, s.childGroups()) {
        s.beginGroup(pluginId);
        bool    validMeta = s.value("metaversion").toInt() == MetadataVersion;
        QString fileName  = s.value("filename").toString();
        if (!validMeta || !QFile::exists(fileName)) {
            s.remove("");
            s.endGroup();
            continue;
        }
        PluginData::Ptr pd(new PluginData);
        pd->fileName            = fileName;
        tmpPlugins[pluginId]    = pd;
        file2data[pd->fileName] = pd;
        pd->loadPolicy          = (LoadPolicy)s.value("loadPolicy").toInt();
        pd->loadStatus          = LS_Undefined;
        pd->modifyTime
            = QDateTime::fromSecsSinceEpoch(s.value("lastModify").toUInt()); // if 0 then we have staled cache. it's ok
        pd->features             = (PluginFeatures)s.value("features").toUInt();
        pd->metadata.id          = pluginId;
        pd->metadata.name        = s.value("name").toString();
        pd->metadata.description = s.value("description").toString();
        pd->metadata.author      = s.value("author").toString();
        pd->metadata.version     = s.value("version").toUInt();
        pd->metadata.minVersion  = s.value("minVersion").toUInt();
        pd->metadata.maxVersion  = s.value("maxVersion").toUInt();
        pd->metadata.extra       = s.value("extra").toHash();
        pd->metadata.icon        = QIcon(iconsCacheDir() + pluginId + QLatin1String(".png"));
        // pd->metadata.extra = s.value("extra").();
        s.endGroup();
    }
    s.endGroup();

    PluginsIterator it;
    while (!it.isFinished()) {
        QString         fileName = it.fileName();
        PluginData::Ptr pd       = file2data.value(fileName);
        if (pd.isNull()
            || (!(pd->loadStatus && pd->loadStatus < LS_Errors)
                && pd->modifyTime < QFileInfo(pd->fileName).lastModified())) { // have to update metadata cache

            loadPlugin(fileName, pd, QLibrary::ExportExternalSymbolsHint);
            if (!pd.isNull()) { // new cache
                tmpPlugins.insert(pd->metadata.id, pd);
            }
        } else {
            // cache is not null and filetimestamp is the same
            tmpPlugins.insert(pd->metadata.id, pd);
        }

        it.next();
    }
    plugins = tmpPlugins;
}

PluginManager::LoadStatus PluginManager::loadPlugin(const QString &fileName, PluginData::Ptr &cache,
                                                    QLibrary::LoadHints loadHints)
{
#ifdef QTNOTE_DEVEL
    qDebug("Loading plugin: %s", qPrintable(fileName));
#endif
    QPluginLoader loader(fileName);
    loader.setLoadHints(loadHints);
    QSettings s;
    s.beginGroup("plugins");
    QObject *plugin = loader.instance();
    if (plugin) {
        PluginInterface *qnp = qobject_cast<PluginInterface *>(plugin);
        if (!qnp) {
            loader.unload();
            qDebug("not QtNote plugin %s. ignore it", qPrintable(fileName));
            return LS_ErrAbi;
        }

        PluginMetadata md           = qnp->metadata();
        bool           metaVerMatch = (qnp->metadataVersion() == MetadataVersion);
        if (!metaVerMatch || md.id.isEmpty() || md.name.isEmpty()) {
            loader.unload();
            if (metaVerMatch) {
                qDebug("QtNote plugin %s did not set metadata id or name. ignore it", qPrintable(fileName));
            } else {
                qDebug("Metadata version of QtNote plugin %s is incompatible", qPrintable(fileName));
            }
            return LS_ErrMetadata;
        }

        LoadStatus loadStatus = LS_Loaded;
        if ((QTNOTE_VERSION < md.minVersion) || (QTNOTE_VERSION > md.maxVersion)) {
            loader.unload();
            qDebug("Incompatible version of qtnote plugin %s. ignore it", qPrintable(fileName));
            return LS_ErrVersion;
        }

        if (!cache) {
            cache             = PluginData::Ptr(new PluginData);
            cache->loadPolicy = LP_Auto;
        }
        cache->instance   = loadStatus == LS_Loaded ? plugin : 0;
        cache->fileName   = fileName;
        cache->modifyTime = QFileInfo(fileName).lastModified();
        cache->metadata   = md;
        cache->features &= 0;
        if (qobject_cast<TrayInterface *>(plugin)) {
            cache->features |= TrayIcon;
        }
        if (qobject_cast<DEIntegrationInterface *>(plugin)) {
            cache->features |= DEIntegration;
        }
        if (qobject_cast<GlobalShortcutsInterface *>(plugin)) {
            cache->features |= GlobalShortcuts;
        }
        if (qobject_cast<NotificationInterface *>(plugin)) {
            cache->features |= Notifications;
        }
        if (qobject_cast<RegularPluginInterface *>(plugin)) {
            cache->features |= RegularPlugin;
        }
        s.beginGroup(cache->metadata.id);
        s.setValue("metaversion", qnp->metadataVersion());
        s.setValue("id", md.id);
        s.setValue("loadPolicy", cache->loadPolicy);
        s.setValue("filename", cache->fileName);
        s.setValue("lastModify", cache->modifyTime.toSecsSinceEpoch());
        s.setValue("features", (uint)cache->features);
        s.setValue("name", md.name);
        s.setValue("description", md.description);
        s.setValue("author", md.author);
        s.setValue("version", md.version);
        s.setValue("minVersion", md.minVersion);
        s.setValue("maxVersion", md.maxVersion);
        s.setValue("extra", md.extra);
        if (!cache->metadata.icon.isNull()) {
            cache->metadata.icon.pixmap(16, 16).save(iconsCacheDir() + md.id + QLatin1String(".png"));
        }
        if (cache->loadPolicy == LP_Disabled || loadHints & QLibrary::ExportExternalSymbolsHint) {
            loader.unload();
            loadStatus = LS_Unloaded; // actual status knows only QPluginLoader. probably I should fix it
        }
        cache->loadStatus = loadStatus;

        if (loadStatus == LS_Loaded) {
            qnp->setHost(pluginHost);
        }

        return loadStatus;
    }
    qDebug("failed to load %s : %s", qPrintable(fileName), qPrintable(loader.errorString()));
    return LS_ErrNotPlugin;
}

} // namespace QtNote
