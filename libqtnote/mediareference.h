#ifndef MEDIAREFERENCE_H
#define MEDIAREFERENCE_H

#include "qtnote_export.h"

#include <QByteArray>
#include <QString>
#include <QUuid>
#include <QVariantMap>

namespace QtNote {

struct QTNOTE_EXPORT MediaReference {
    QUuid       id;
    QByteArray  blobId;
    QString     originalName;
    QString     portableName;
    QString     mediaType;
    qint64      size { 0 };
    QByteArray  checksum;
    QVariantMap remoteData;

    QString uri() const;
    bool    isValid() const { return !id.isNull() && !blobId.isEmpty() && !portableName.isEmpty(); }
};

} // namespace QtNote

#endif // MEDIAREFERENCE_H
