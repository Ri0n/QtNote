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

#ifndef TOMBOYDATA_H
#define TOMBOYDATA_H

#include "notedata.h"

#include <QDateTime>
#include <QDomDocument>
#include <QFile>
#include <QPointer>

namespace QtNote {

class TomboyData : public QtNote::NoteData {
public:
    using NoteData::NoteData;

    bool fromFile(QString);
    bool saveToFile(const QString &fileName);

    QString storageId() const override;
    bool    load() override;
    void    remove() override;

    QString     nodeText(QDomNode node) const;
    QStringList tagsFromNode(QDomNode node) const;
    QStringList searchableTags(const QStringList &tags) const;

    QString sFileName;

protected:
    QDateTime   dtCreate;
    int         iCursor;
    int         iWidth;
    int         iHeight;
    QStringList xmlTags_;
};

} // namespace QtNote

#endif // TOMBOYDATA_H
