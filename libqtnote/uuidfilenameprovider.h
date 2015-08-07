#ifndef QTNOTE_UUIDFILENAMEPROVIDER_H
#define QTNOTE_UUIDFILENAMEPROVIDER_H

#include "filenameprovider.h"

namespace QtNote {

class UuidFileNameProvider : public FileNameProvider
{
public:
    using FileNameProvider::FileNameProvider;

    QString newName(const Note &note, QString &noteId);
    QString updateName(const Note &note, QString &noteId);
};

} // namespace QtNote

#endif // QTNOTE_UUIDFILENAMEPROVIDER_H
