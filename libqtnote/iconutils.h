#ifndef ICONUTILS_H
#define ICONUTILS_H

#include <QString>

#include "qtnote_export.h"

class QColor;
class QIcon;

namespace QtNote {

class QTNOTE_EXPORT IconUtils {
public:
    static bool  isDarkColorScheme();
    static QIcon tintedSymbolicIcon(const QString &path, const QColor &color);
    static QIcon symbolicIcon(const QString &path);
    static QIcon themedIcon(const QString &name, const QString &fallbackPath);
};

} // namespace QtNote

#endif // ICONUTILS_H
