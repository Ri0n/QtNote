#ifndef LOCALDATAKEYSTORE_H
#define LOCALDATAKEYSTORE_H

#include "qtnote_export.h"

#include <QByteArray>
#include <QString>

namespace QtNote {

class QTNOTE_EXPORT LocalDataKeyStore {
public:
    static constexpr int MasterKeySize = 32;

    static QByteArray loadOrCreateMasterKey(QString *error = nullptr);
};

} // namespace QtNote

#endif // LOCALDATAKEYSTORE_H
