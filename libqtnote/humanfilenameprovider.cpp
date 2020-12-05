#include <QString>

#include "filenotedata.h"
#include "humanfilenameprovider.h"

namespace QtNote {

QString HumanFileNameProvider::fnSearch(const FileNoteData &note, QString &noteId)
{
    QString pfix = QLatin1Char('.') + fileExt;
    QString fileName;
    QString title = note.title();
    QRegExp r("[<>:\"/\\\\|?*]");
    title = title.replace(r, QChar('_')).left(FN_MAX_LEN - pfix.size());

    if (title != noteId) { // filename shoud be changed or it's new note
        QString suf;
        int     ind = 0;

        QString proposedId = title;
        while (dir.exists((fileName = dir.absoluteFilePath(QString("%1%2").arg(proposedId, pfix))))) {
            ind++;
            suf        = QString::number(ind);
            proposedId = title.left(FN_MAX_LEN - suf.size() - pfix.size()) + suf;
        }
        noteId = proposedId;
    } else {
        fileName = dir.absoluteFilePath(QString("%1.%3").arg(title, fileExt));
    }
    return fileName;
}

QString HumanFileNameProvider::newName(const FileNoteData &note, QString &noteId) { return fnSearch(note, noteId); }

QString HumanFileNameProvider::updateName(const FileNoteData &note, QString &noteId) { return fnSearch(note, noteId); }

} // namespace QtNote
