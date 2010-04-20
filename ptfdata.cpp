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
		: NoteData()
{
	sTitle = "(no name)";
}

bool PTFData::fromFile(QString fn)
{
	QFile file(fn);
	if (!file.open(QIODevice::ReadOnly))
		return false;
	setFile(fn);
	file.close();

	return true;
}

void PTFData::setFile(QString fn)
{
	Q_UNUSED(fn)
}

QString PTFData::title() const
{
	return sTitle;
}

QDateTime PTFData::modifyTime() const
{
	return dtLastChange;
}

QString PTFData::text() const
{
	return sText;
}

void PTFData::setText(const QString &text)
{
	sText = text;
	sTitle = sText.section('\n', 0, 0); //FIXME crossplatform?
}

void PTFData::toTrash()
{
	QFile f(sFileName);
	f.remove();
	//emit
}

void PTFData::saveToFile(const QString &fileName)
{
	Q_UNUSED(fileName)
}
