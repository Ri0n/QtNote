#ifndef TRAYICONUTILS_H
#define TRAYICONUTILS_H

#include <QString>

#include "qtnote_export.h"

class QIcon;
class QSystemTrayIcon;

namespace QtNote {

class QTNOTE_EXPORT TrayIconUtils {
public:
    static QString themedTrayIconName();
    static QIcon   themedTrayIcon();
    static void    setupSystemTrayIcon(QSystemTrayIcon *trayIcon);
};

} // namespace QtNote

#endif // TRAYICONUTILS_H
