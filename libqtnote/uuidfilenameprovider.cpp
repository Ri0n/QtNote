#include <QUuid>

#include "uuidfilenameprovider.h"

namespace QtNote {

QString UuidFileNameProvider::newName(const Note &note, QString &noteId)
{
    QString uid = QUuid::createUuid().toString();
    noteId = uid.mid(1, uid.length()-2);
    return dir.absoluteFilePath(QString("%1.%2").arg(noteId, fileExt));
}

QString UuidFileNameProvider::updateName(const Note &note, QString &noteId)
{
    return dir.absoluteFilePath(QString("%1.%2").arg(noteId, fileExt));
}

} // namespace QtNote

