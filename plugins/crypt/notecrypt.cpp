#include <QtPlugin>

#include "notecrypt.h"

NoteCrypt::NoteCrypt(QObject *parent) :
    QObject(parent)
{
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(crypt, NoteCrypt)
#endif
