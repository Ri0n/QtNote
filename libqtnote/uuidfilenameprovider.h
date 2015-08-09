#ifndef QTNOTE_UUIDFILENAMEPROVIDER_H
#define QTNOTE_UUIDFILENAMEPROVIDER_H

#include "filenameprovider.h"

namespace QtNote {

class QTNOTE_EXPORT UuidFileNameProvider : public FileNameProvider
{
public:
    //using FileNameProvider::FileNameProvider; // it's not supported with vs2013
    inline UuidFileNameProvider(const QString &path, const QString &fileExt) :
        FileNameProvider(path, fileExt) {}

    QString newName(const FileNoteData &note, QString &noteId);
    QString updateName(const FileNoteData &note, QString &noteId);
};

} // namespace QtNote

#endif // QTNOTE_UUIDFILENAMEPROVIDER_H
