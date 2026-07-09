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

#include "ptfdata.h"
#include "notemanager.h"
#include "ptfstorage.h"
#include "utils.h"

#include <QFileInfo>
#include <QIcon>

namespace QtNote {

bool PTFData::fromFile(const QString &fileName)
{
    QFileInfo fi(fileName);
    QFile     file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    id_ = fi.completeBaseName();
    format_
        = fi.suffix().toLower().endsWith(QLatin1String("md"), Qt::CaseInsensitive) ? Note::Markdown : Note::PlainText;
    title_ = QString::fromUtf8(file.readLine()).trimmed();

    sFileName = fileName;
    file.close();

    lastChange_ = fi.lastModified();

    return true;
}

bool PTFData::saveToFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("Failed to write: %s\n", qPrintable(file.errorString()));
        return false;
    }
    auto data = (title_ + QLatin1Char('\n') + text_).trimmed();
    file.write(data.toUtf8());
    sFileName = fileName;
    file.close();
    lastChange_ = QFileInfo(file).lastModified();
    return true;
}

QString PTFData::storageId() const { return PTFStorage::storageId; }

bool PTFData::load()
{
    auto  fs   = static_cast<FileStorage *>(storage_);
    auto  data = fs->loadNoteData(*this);
    QFile file(sFileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    auto data               = QString::fromUtf8(file.readAll());
    std::tie(title_, text_) = Utils::splitTitle(data);
    loaded_                 = true;
    return true;
}

bool PTFData::save()
{
    if (!loaded_) {
        qWarning("can't save not loaded note");
        return false;
    }
    return storage_->saveNote(this);
}

void PTFData::remove() { storage_->removeNote(id_); }

} // namespace QtNote
