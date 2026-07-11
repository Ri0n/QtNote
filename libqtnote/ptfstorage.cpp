/*
QtNote - Simple note-taking application
Copyright (C) 2010 Sergei Ilinykh

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Contacts:
E-Mail: rion4ik@gmail.com XMPP: rion@jabber.ru
*/

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QSettings>
#include <QStyle>

#ifdef Q_OS_WIN
#include <QLibrary>
#include <shlobj.h>
#endif // Q_OS_WIN

#include "notedata.h"
#include "ptfstorage.h"
#include "utils.h"

namespace QtNote {

PTFStorage::PTFStorage(QObject *parent) : FileStorage(parent), icon(QLatin1String(":/icons/trayicon"))
{
    fileExt.append(QLatin1String("txt"));
    fileExt.append(QLatin1String("md"));
}

bool PTFStorage::init()
{
    auto path = QSettings().value("storage.ptf.path").toString();
    notesDir.setPath(path.isEmpty() ? findStorageDir() : path);
    if (!notesDir.isReadable() && !path.isEmpty()) {
        notesDir.setPath(findStorageDir()); // try default
    }
    if (!notesDir.exists()) {
        if (!notesDir.mkpath(QLatin1String("."))) {
            qWarning("can't create storage dir: %s", qPrintable(notesDir.absolutePath()));
        }
    }
    return isAccessible();
}

bool PTFStorage::isAccessible() const { return notesDir.isReadable(); }

const QString PTFStorage::systemName() const { return storageId; }

const QString PTFStorage::name() const { return tr("Plain Text Storage"); }

QIcon PTFStorage::storageIcon() const { return icon; }

QIcon PTFStorage::noteIcon() const { return icon; }

QList<Note> PTFStorage::noteListFromInfoList(const QFileInfoList &files)
{
    QList<Note> ret;
    foreach (const QFileInfo &fi, files) {
        QFile file(fi.canonicalFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        Note note(new NoteData(this));
        note.setId(fi.completeBaseName());
        note.setFormat(fi.suffix().compare(QLatin1String("md"), Qt::CaseInsensitive) == 0 ? Note::Markdown
                                                                                          : Note::PlainText);
        note.setTitle(QString::fromUtf8(file.readLine()).trimmed());
        note.setTags(NoteData::tagsFromLine(QString::fromUtf8(file.readLine())));
        note.setLastChangeUTC(fi.lastModified());
        note.setBackendValue(QStringLiteral("fileName"), fi.canonicalFilePath());
        note.unload();
        ret.append(note);
    }
    return ret;
}

Note PTFStorage::createNote()
{
    Note note(new NoteData(this));
    note.setFormat(Note::PlainText);
    note.setText(QString(), Note::PlainText);
    note.setLastChangeUTC(QDateTime::currentDateTimeUtc());
    return note;
}

Note PTFStorage::note(const QString &noteId)
{
    if (!noteId.isEmpty()) {
        QFileInfo fi;
        for (auto const &ext : std::as_const(fileExt)) {
            fi = QFileInfo(QDir(notesDir).absoluteFilePath(QString("%1.%2").arg(noteId, ext)));
            if (fi.exists() || fi.isWritable()) {
                Note loaded(new NoteData(this));
                loaded.setId(fi.completeBaseName());
                loaded.setFormat(fi.suffix().compare(QLatin1String("md"), Qt::CaseInsensitive) == 0 ? Note::Markdown
                                                                                                    : Note::PlainText);
                loaded.setLastChangeUTC(fi.lastModified());
                loaded.setBackendValue(QStringLiteral("fileName"), fi.filePath());
                if (loadNote(loaded))
                    return loaded;
                return {};
            }
        }
        handleFSError();
    }
    return Note();
}

bool PTFStorage::loadNote(Note &note)
{
    QString fileName = note.backendValue(QStringLiteral("fileName")).toString();
    if (fileName.isEmpty()) {
        for (const auto &ext : std::as_const(fileExt)) {
            const auto candidate = notesDir.absoluteFilePath(QStringLiteral("%1.%2").arg(note.id(), ext));
            if (QFileInfo::exists(candidate)) {
                fileName = candidate;
                break;
            }
        }
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    const auto [title, body] = Utils::splitTitle(QString::fromUtf8(file.readAll()));
    note.setTitle(title);
    note.setText(body,
                 QFileInfo(fileName).suffix().compare(QLatin1String("md"), Qt::CaseInsensitive) == 0 ? Note::Markdown
                                                                                                     : Note::PlainText);
    note.setLastChangeUTC(QFileInfo(file).lastModified());
    note.setBackendValue(QStringLiteral("fileName"), fileName);
    return true;
}

bool PTFStorage::saveNote(const Note &note)
{
    if (note.isEmpty()) {
        if (!note.id().isEmpty()) {
            removeNote(note.id()); // TODO check errors?
        }
        return true;
    }

    QString oldNoteId = note.id();
    QString newNoteId = oldNoteId;
    auto    ext       = QString(QLatin1String(note.format() == Note::Markdown ? "md" : "txt"));
    auto    fileName  = Utils::fileNameForText(notesDir, note.title(), ext, newNoteId);
    QFile   file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)
        || file.write((note.title() + QLatin1Char('\n') + note.text()).trimmed().toUtf8()) < 0) {
        handleFSError();
        return false;
    }
    file.close();
    Note saved = note;
    saved.setId(newNoteId);
    saved.setLastChangeUTC(QFileInfo(fileName).lastModified());
    saved.setBackendValue(QStringLiteral("fileName"), fileName);
    if (!oldNoteId.isEmpty()) {
        if (oldNoteId != newNoteId) {
            for (auto const &ext : std::as_const(fileExt)) {
                notesDir.remove(oldNoteId + QLatin1Char('.') + ext);
            }
        } else {
            for (auto const &otherExt : std::as_const(fileExt)) {
                if (ext != otherExt) {
                    notesDir.remove(oldNoteId + QLatin1Char('.') + otherExt);
                }
            }
        }
    }
    putToCache(saved, oldNoteId);
    return true;
}

QList<Note::Format> PTFStorage::availableFormats() const
{
    static auto formats = QList<Note::Format>() << Note::Markdown << Note::PlainText;
    return formats;
}

QString PTFStorage::findStorageDir() const { return Utils::qtnoteDataDir() + QLatin1Char('/') + storageId; }

QString PTFStorage::storageId = QStringLiteral("ptf");

} // namespace QtNote
