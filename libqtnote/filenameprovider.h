#ifndef FILENAMEPROVIDER_H
#define FILENAMEPROVIDER_H

#include <QString>
#include <QDir>

#include "qtnote_export.h"

namespace QtNote {

class FileNoteData;

class QTNOTE_EXPORT FileNameProvider
{
protected:
    QDir dir;
    QString fileExt;
    bool valid = false;

public:
    inline FileNameProvider(const QString &path, const QString &fileExt) :
        fileExt(fileExt) { setPath(path); }
    virtual ~FileNameProvider() {}
    inline bool isValid() const { return valid && dir.exists(); }
    inline bool setPath(const QString &path) { dir = path; valid = !path.isEmpty() && dir.exists(); return valid; }

    virtual QString newName(const FileNoteData &note, QString &noteId) = 0;
    virtual QString updateName(const FileNoteData &note, QString &noteId) = 0;
    virtual QString uidForFileName(const QString &fileName);
    virtual QString fileNameForUid(const QString &noteId);
};

} // namespace QtNote

#endif // FILENAMEPROVIDER_H
