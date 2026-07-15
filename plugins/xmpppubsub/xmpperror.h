#ifndef XMPPERROR_H
#define XMPPERROR_H

#include "xmppdto.h"

struct QXmppError;

namespace QtNote {

/**
 * @brief Maps a QXmpp protocol error to QtNote's retry/error-state policy.
 * @param error Error returned by a QXmpp asynchronous operation.
 */
XmppErrorKind classifyXmppError(const QXmppError &error);

} // namespace QtNote

#endif // XMPPERROR_H
