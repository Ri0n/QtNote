#ifndef XMPPERROR_H
#define XMPPERROR_H

#include "xmppdto.h"

struct QXmppError;

namespace QtNote {

XmppErrorKind classifyXmppError(const QXmppError &error);

} // namespace QtNote

#endif // XMPPERROR_H
