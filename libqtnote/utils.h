/*
QtNote - Simple note-taking application
Copyright (C) 2010 Ili'nykh Sergey

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

#ifndef UTILS_H
#define UTILS_H

#include <QString>

#include "qtnote_export.h"

class QColor;

class QTNOTE_EXPORT Utils
{
public:
    Utils();

    static QString cuttedDots(const QString &, int);

    static const QString genericDataDir();
    static const QString &qtnoteDataDir();

    static QColor perceptiveColor(const QColor &against);
    static QColor mergeColors(const QColor &a, const QColor &b);
};

#endif // UTILS_H
