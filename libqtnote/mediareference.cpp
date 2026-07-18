#include "mediareference.h"

#include <QUrl>

namespace QtNote {

QString MediaReference::uri() const
{
    return QStringLiteral("qtnote-media:/%1/%2")
        .arg(id.toString(QUuid::WithoutBraces), QString::fromUtf8(QUrl::toPercentEncoding(portableName)));
}

} // namespace QtNote
