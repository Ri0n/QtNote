#ifndef FILENAMEPROVIDER_H
#define FILENAMEPROVIDER_H

#include <QString>
#include <QDir>

namespace QtNote {

class Note;

class FileNameProvider
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

    virtual QString newName(const Note &note, QString &noteId) = 0;
    virtual QString updateName(const Note &note, QString &noteId) = 0;
};

} // namespace QtNote

#endif // FILENAMEPROVIDER_H
