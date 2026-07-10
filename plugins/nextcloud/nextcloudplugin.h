#ifndef NEXTCLOUDPLUGIN_H
#define NEXTCLOUDPLUGIN_H

#include "qtnoteplugininterface.h"

#include <QObject>

namespace QtNote {

class Main;
class PluginHostInterface;

class NextcloudPlugin final : public QObject, public PluginInterface, public RegularPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.nextcloud")
    Q_INTERFACES(QtNote::PluginInterface QtNote::RegularPluginInterface)

public:
    explicit NextcloudPlugin(QObject *parent = nullptr);
    ~NextcloudPlugin() override;

    int            metadataVersion() const override;
    void           setHost(PluginHostInterface *host) override;
    PluginMetadata metadata() override;
    bool           init(Main *qtnote) override;

private:
    Main                *qtnote_ { nullptr };
    PluginHostInterface *host_ { nullptr };
};

} // namespace QtNote

#endif // NEXTCLOUDPLUGIN_H
