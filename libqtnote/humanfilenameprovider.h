#ifndef QTNOTE_HUMANFILENAMEPROVIDER_H
#define QTNOTE_HUMANFILENAMEPROVIDER_H

#include "filenameprovider.h"

namespace QtNote {

class HumanFileNameProvider : public FileNameProvider
{
public:
    HumanFileNameProvider(const QString &path, const QString &fileExt) :
        FileNameProvider(path, fileExt) {}

    QString newName(const Note &note, QString &noteId);
    QString updateName(const Note &note, QString &noteId);
private:
    QString fnSearch(const Note &note, QString &noteId);
};

} // namespace QtNote

#endif // QTNOTE_HUMANFILENAMEPROVIDER_H
