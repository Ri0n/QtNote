#include "nextcloudplugin.h"

#include "nextcloudstorage.h"
#include "pluginhostinterface.h"
#include "qtnote.h"
#include "qtnote_config.h"

namespace QtNote {

namespace {

    const QLatin1String pluginId("nextcloud_storage");
    NoteStorage::Ptr    storage;

} // namespace

NextcloudPlugin::NextcloudPlugin(QObject *parent) : QObject(parent) { }

NextcloudPlugin::~NextcloudPlugin() { deinit(); }

int NextcloudPlugin::metadataVersion() const { return MetadataVersion; }

void NextcloudPlugin::setHost(PluginHostInterface *host) { host_ = host; }

PluginMetadata NextcloudPlugin::metadata()
{
    PluginMetadata metadata;
    metadata.id          = pluginId;
    metadata.icon        = QIcon::fromTheme(QStringLiteral("folder-cloud"));
    metadata.name        = QString("Nextcloud Notes");
    metadata.description = tr("Reads and writes notes using the Nextcloud Notes REST API");
    metadata.author      = QString("Sergei Ilinykh");
    metadata.version     = 0x010000;
    metadata.minVersion  = 0x020300;
    metadata.maxVersion  = QTNOTE_VERSION;
    metadata.homepage    = QUrl(QStringLiteral("https://github.com/nextcloud/notes"));
    return metadata;
}

bool NextcloudPlugin::init(Main *qtnote)
{
    deinit();
    qtnote_           = qtnote;
    auto ownedStorage = std::make_unique<NextcloudStorage>(nullptr);
    storage           = ownedStorage.get();
    qtnote_->registerStorage(std::move(ownedStorage));

    // A remote storage must remain enabled while it is not configured,
    // otherwise the user cannot reach its settings widget.
    return true;
}

void NextcloudPlugin::deinit()
{
    if (qtnote_) {
        qtnote_->unregisterStorage(storage.data());
        storage.clear();
        qtnote_ = nullptr;
    }
}

} // namespace QtNote
