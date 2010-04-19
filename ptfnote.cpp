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

#include "ptfnote.h"
#include <QIcon>
#include <QtDebug>

class PTFNote::Private : public QSharedData
{
public:
	QString fileName;
	QString title;
	QString text;
	QDateTime dtLastChange;
	QDateTime dtCreate;
};

PTFNote::PTFNote()
		: Note()
{
	d = new Private;
	d->title = "(no name)";
}

PTFNote::PTFNote(const PTFNote &other)
	: d (other.d)
{

}

PTFNote::~PTFNote()
{

}


bool PTFNote::fromFile(QString fn)
{
	QFile file(fn);
	if (!file.open(QIODevice::ReadOnly))
		return false;
	setFile(fn);
	file.close();

	return true;
}

void PTFNote::setFile(QString fn)
{
	Q_UNUSED(fn)
}

QString PTFNote::title() const
{
	return d->title;
}

QDateTime PTFNote::modifyTime() const
{
	return d->dtLastChange;
}

QString PTFNote::text() const
{
	return d->text;
}

void PTFNote::setText(const QString &text)
{
	d->text = text;
	d->title = d->text.section('\n', 0, 0); //FIXME crossplatform?
}

void PTFNote::toTrash()
{
	QFile f(d->fileName);
	f.remove();
	//emit
}

void PTFNote::saveToFile(const QString &fileName)
{
	Q_UNUSED(fileName)
}
