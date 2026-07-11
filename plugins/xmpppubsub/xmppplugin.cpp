#include "xmppplugin.h"

#include "pluginhostinterface.h"
#include "qtnote.h"
#include "qtnote_config.h"
#include "xmppstorage.h"

namespace QtNote {

namespace {

    const QLatin1String pluginId("xmpp_pubsub_storage");
    NoteStorage::Ptr    storage;

} // namespace

XmppPlugin::XmppPlugin(QObject *parent) : QObject(parent) { }

XmppPlugin::~XmppPlugin() { deinit(); }

int XmppPlugin::metadataVersion() const { return MetadataVersion; }

void XmppPlugin::setHost(PluginHostInterface *host) { host_ = host; }

PluginMetadata XmppPlugin::metadata()
{
    PluginMetadata metadata;
    metadata.id          = pluginId;
    metadata.icon        = QIcon::fromTheme(QStringLiteral("im-jabber"));
    metadata.name        = QStringLiteral("XMPP Private Notes");
    metadata.description = tr("Stores notes as private persistent items in the account's XMPP PEP service");
    metadata.author      = QStringLiteral("Sergei Ilinykh");
    metadata.version     = 0x010000;
    metadata.minVersion  = 0x020300;
    metadata.maxVersion  = QTNOTE_VERSION;
    metadata.homepage    = QUrl(QStringLiteral("https://xmpp.org/extensions/xep-0223.html"));
    return metadata;
}

bool XmppPlugin::init(Main *qtnote)
{
    deinit();
    qtnote_ = qtnote;
    storage = NoteStorage::Ptr(new XmppStorage(this));
    qtnote_->registerStorage(storage);

    // Keep an unconfigured remote storage enabled so its settings remain reachable.
    return true;
}

void XmppPlugin::deinit()
{
    if (qtnote_) {
        qtnote_->unregisterStorage(storage);
        storage.clear();
        qtnote_ = nullptr;
    }
}

} // namespace QtNote
