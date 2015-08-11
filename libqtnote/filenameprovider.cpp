#include "filenameprovider.h"


// TODO review if we need this file in future

namespace QtNote {

QString FileNameProvider::uidForFileName(const QString &fileName)
{
    QFileInfo fi(fileName);
    return fi.completeBaseName();
}

QString FileNameProvider::fileNameForUid(const QString &noteId)
{
    return dir.absoluteFilePath(noteId + QLatin1Char('.') + fileExt);
}


} // namespace QtNote
