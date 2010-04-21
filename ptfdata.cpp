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

#include "ptfdata.h"
#include <QIcon>
#include <QtDebug>

PTFData::PTFData()
		: FileNoteData()
{

}

bool PTFData::fromFile(QString fn)
{
	QFile file(fn);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	setText(file.readAll());
	setFile(fn);
	file.close();

	return true;
}

bool PTFData::saveToFile(const QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	file.write(sText.toUtf8());
	setFile(fileName);
	file.close();
	return true;
}
