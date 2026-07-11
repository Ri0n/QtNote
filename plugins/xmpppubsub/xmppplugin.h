#ifndef XMPPPLUGIN_H
#define XMPPPLUGIN_H

#include "qtnoteplugininterface.h"

#include <QObject>

namespace QtNote {

class Main;
class PluginHostInterface;

class XmppPlugin final : public QObject, public PluginInterface, public RegularPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.xmpppubsub")
    Q_INTERFACES(QtNote::PluginInterface QtNote::RegularPluginInterface)

public:
    explicit XmppPlugin(QObject *parent = nullptr);
    ~XmppPlugin() override;

    int            metadataVersion() const override;
    void           setHost(PluginHostInterface *host) override;
    PluginMetadata metadata() override;
    bool           init(Main *qtnote) override;
    void           deinit() override;

private:
    Main                *qtnote_ { nullptr };
    PluginHostInterface *host_ { nullptr };
};

} // namespace QtNote

#endif // XMPPPLUGIN_H
