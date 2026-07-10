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

NextcloudPlugin::~NextcloudPlugin()
{
    if (qtnote_) {
        qtnote_->unregisterStorage(storage);
        storage.clear();
    }
}

int NextcloudPlugin::metadataVersion() const { return MetadataVersion; }

void NextcloudPlugin::setHost(PluginHostInterface *host) { host_ = host; }

PluginMetadata NextcloudPlugin::metadata()
{
    PluginMetadata metadata;
    metadata.id          = pluginId;
    metadata.icon        = QIcon::fromTheme(QStringLiteral("folder-cloud"));
    metadata.name        = QStringLiteral("Nextcloud Notes");
    metadata.description = tr("Reads and writes notes using the Nextcloud Notes REST API");
    metadata.author      = QStringLiteral("Sergei Ilinykh");
    metadata.version     = 0x010000;
    metadata.minVersion  = 0x020300;
    metadata.maxVersion  = QTNOTE_VERSION;
    metadata.homepage    = QUrl(QStringLiteral("https://github.com/nextcloud/notes"));
    return metadata;
}

bool NextcloudPlugin::init(Main *qtnote)
{
    qtnote_ = qtnote;
    storage = NoteStorage::Ptr(new NextcloudStorage(this));
    qtnote_->registerStorage(storage);

    // A remote storage must remain enabled while it is not configured,
    // otherwise the user cannot reach its settings widget.
    return true;
}

} // namespace QtNote
