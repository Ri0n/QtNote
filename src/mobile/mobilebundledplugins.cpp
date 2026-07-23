#include "mobilebundledplugins.h"

#include "bundledpluginregistry.h"

namespace QtNote {

void registerMobileBundledPlugins(BundledPluginRegistry &registry)
{
    Q_UNUSED(registry)
    // Android plugins are registered explicitly here. A plugin becomes
    // eligible only after its runtime implementation no longer depends on
    // QWidget, QDialog, Main, dynamic desktop discovery, or DBus.
}

} // namespace QtNote
