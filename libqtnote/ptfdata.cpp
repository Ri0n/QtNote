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
#include <QFileInfo>
#include <QIcon>

namespace QtNote {

bool PTFData::fromFile(QString fn)
{
    QFile file(fn);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    setText(QString::fromUtf8(file.readAll()));
    sFileName = fn;
    file.close();
    QFileInfo fi(fn);
    dtCreate     = fi.birthTime();
    dtLastChange = fi.lastModified();

    return true;
}

bool PTFData::saveToFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("Failed to write: %s\n", qPrintable(file.errorString()));
        return false;
    }
    file.write(sText.toUtf8());
    sFileName = fileName;
    file.close();
    dtLastChange = QFileInfo(file).lastModified();
    return true;
}

void PTFData::remove() { QFile(sFileName).remove(); }

qint64 PTFData::lastChangeElapsed() const { return dtLastChange.msecsTo(QDateTime::currentDateTime()); }

} // namespace QtNote
