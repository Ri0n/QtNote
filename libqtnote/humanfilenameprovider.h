#ifndef QTNOTE_HUMANFILENAMEPROVIDER_H
#define QTNOTE_HUMANFILENAMEPROVIDER_H

#include "filenameprovider.h"

namespace QtNote {

class HumanFileNameProvider : public FileNameProvider
{
    static const int FN_MAX_LEN = 255;
public:
    HumanFileNameProvider(const QString &path, const QString &fileExt) :
        FileNameProvider(path, fileExt) {}

    QString newName(const FileNoteData &note, QString &noteId);
    QString updateName(const FileNoteData &note, QString &noteId);
private:
    QString fnSearch(const FileNoteData &note, QString &noteId);
};

} // namespace QtNote

#endif // QTNOTE_HUMANFILENAMEPROVIDER_H
