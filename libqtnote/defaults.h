#ifndef DEFAULTS_H
#define DEFAULTS_H

#include <QColor>

namespace QtNote {

class Defaults {
public:
    static inline QColor firstLineHighlightColor() { return QColor(255, 0, 0, 180); }
};

}

#endif // DEFAULTS_H
