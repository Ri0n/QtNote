/*
    SPDX-License-Identifier: GPL-3.0-only
*/

#ifndef FREEDESKTOPNOTIFIER_H
#define FREEDESKTOPNOTIFIER_H

#include "qtnote_export.h"

#include <QString>

namespace QtNote {

class QTNOTE_EXPORT FreedesktopNotifier {
public:
    static bool notifyError(const QString &summary, const QString &body);
};

} // namespace QtNote

#endif // FREEDESKTOPNOTIFIER_H
