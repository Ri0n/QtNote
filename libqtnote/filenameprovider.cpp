#include "filenameprovider.h"


// TODO review if we need this file in future

namespace QtNote {

QString FileNameProvider::uidForFileName(const QString &fileName)
{
    QFileInfo fi(fileName);
    return fi.completeBaseName();
}


} // namespace QtNote
