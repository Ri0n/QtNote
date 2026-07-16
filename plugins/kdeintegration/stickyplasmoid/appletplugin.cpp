#include <KPluginFactory>
#include <Plasma/Applet>

class QtNoteStickyPlasmoidPlugin : public Plasma::Applet {
    Q_OBJECT

public:
    QtNoteStickyPlasmoidPlugin(QObject *parent, const KPluginMetaData &data, const QVariantList &args) :
        Plasma::Applet(parent, data, args)
    {
    }
};

K_PLUGIN_CLASS_WITH_JSON(QtNoteStickyPlasmoidPlugin, "pluginmetadata.json")

#include "appletplugin.moc"
