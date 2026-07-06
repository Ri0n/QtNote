#include <KPluginFactory>
#include <Plasma/Applet>

class QtNotePlasmoidPlugin : public Plasma::Applet {
    Q_OBJECT

public:
    QtNotePlasmoidPlugin(QObject *parent, const KPluginMetaData &data, const QVariantList &args) :
        Plasma::Applet(parent, data, args)
    {
    }
};

K_PLUGIN_CLASS_WITH_JSON(QtNotePlasmoidPlugin, "pluginmetadata.json")

#include "appletplugin.moc"
