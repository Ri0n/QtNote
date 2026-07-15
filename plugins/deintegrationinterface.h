#ifndef DEINTEGRATIONINTERFACE_H
#define DEINTEGRATIONINTERFACE_H

#include <QString>

class QWidget;

namespace QtNote {

enum class WindowGeometryRestoreResult { Unsupported, Restored, Pending };

class DEIntegrationInterface {
public:
    virtual void activateWidget(QWidget *w) = 0;

    // Return false when the desktop integration does not support the operation.
    // The caller can then use its platform-independent fallback.
    virtual WindowGeometryRestoreResult restoreWindowGeometry(QWidget *, const QString &)
    {
        return WindowGeometryRestoreResult::Unsupported;
    }
    virtual bool    saveWindowGeometry(QWidget *, const QString &) { return false; }
    virtual bool    removeWindowGeometry(const QString &) { return false; }
    virtual QString takePendingWindowGeometryKey() { return {}; }
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::DEIntegrationInterface, "com.rion-soft.QtNote.DEIntegrationInterface/1.0")

#endif
