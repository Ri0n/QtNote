#include <KPluginFactory>
#include <Plasma/Applet>
#include <Plasma/Containment>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logPlasmoid, "qtnote.plasmoid")

class QtNotePlasmoidPlugin : public Plasma::Applet {
    Q_OBJECT

    Q_PROPERTY(bool inSystemTray READ inSystemTray NOTIFY inSystemTrayChanged)

public:
    QtNotePlasmoidPlugin(QObject *parent, const KPluginMetaData &data, const QVariantList &args) :
        Plasma::Applet(parent, data, args)
    {
        connect(this, &Plasma::Applet::containmentChanged, this, [this] { emit inSystemTrayChanged(); });
    }

    bool inSystemTray() const
    {
        const auto *currentContainment = containment();
        if (!currentContainment) {
            return false;
        }
        // qCInfo(logPlasmoid)
        //     << "containment pluginId=" << currentContainment->pluginMetaData().pluginId();

        return currentContainment->pluginMetaData().pluginId() == QStringLiteral("org.kde.plasma.systemtray");
    }

signals:
    void inSystemTrayChanged();
};

K_PLUGIN_CLASS_WITH_JSON(QtNotePlasmoidPlugin, "pluginmetadata.json")

#include "appletplugin.moc"
