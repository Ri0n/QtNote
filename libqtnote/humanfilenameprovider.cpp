#include <QString>

#include "humanfilenameprovider.h"
#include "note.h"

namespace QtNote {

QString HumanFileNameProvider::fnSearch(const Note &note, QString &noteId)
{
    QString fileName;
    QString title = note.title();
    QRegExp r("[<>:\"/\\\\|?*]");
    title.replace(r, QChar('_'));

    if (title != noteId) { // filename shoud be changed or it's new note
        //d.remove(fileName);

        QString suf;
        int ind = 0;

        while (dir.exists((fileName = dir.absoluteFilePath(QString("%1%2.%3").arg(title, suf, fileExt))))) {
            ind++;
            suf = QString::number(ind);
        }
        noteId = title + suf;
    } else {
        fileName = dir.absoluteFilePath(QString("%1.%3").arg(title, fileExt));
    }
    return fileName;
}

QString HumanFileNameProvider::newName(const Note &note, QString &noteId)
{
    return fnSearch(note, noteId);
}

QString HumanFileNameProvider::updateName(const Note &note, QString &noteId)
{
    return fnSearch(note, noteId);
}

} // namespace QtNote

