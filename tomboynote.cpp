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

#include "tomboynote.h"
#include <QIcon>
#include <QtDebug>

class TomboyNote::Private : public QSharedData
{
public:
	QString sFile;
	QString sUid;
	QString sTitle;
	QString sText;
	QDateTime dtLastChange;
	QDateTime dtCreate;
	int iCursor;
	int iWidth;
	int iHeight;
};


TomboyNote::TomboyNote()
		: Note()
{
	d = new Private;
	d->sTitle = "(no name)";
}

TomboyNote::TomboyNote(const TomboyNote &other)
	: d (other.d)
{

}

TomboyNote::~TomboyNote()
{

}


bool TomboyNote::fromFile(QString fn)
{
	QDomDocument dom("tomboynote");
	QFile file(fn);
	if (!file.open(QIODevice::ReadOnly))
		return false;
	if (!dom.setContent(&file)) {
		file.close();
		return false;
	}
	setFile(fn);
	file.close();

	QDomElement root = dom.documentElement();
	d->sTitle = nodeText(root.namedItem("title"));
	d->sText = nodeText(root.namedItem("text"));
	d->dtLastChange = QDateTime::fromString(nodeText(root.namedItem("last-change-date")), Qt::ISODate);
	//d->dtLastMetadataChange = QDateTime::fromString(nodeText(root.namedItem("last-metadata-change-date")), Qt::ISODate);
	d->dtCreate = QDateTime::fromString(nodeText(root.namedItem("create-date")), Qt::ISODate);
	d->iCursor = nodeText(root.namedItem("create-date")).toInt();
	d->iWidth = nodeText(root.namedItem("width")).toInt();
	d->iHeight = nodeText(root.namedItem("height")).toInt();

	return true;
}

void TomboyNote::setFile(QString fn)
{
	//qDebug("setf file: %s", qPrintable(fn));
	d->sFile = fn;
	int pos = fn.lastIndexOf('/');
	if (pos != -1) {
		fn = fn.mid(pos+1);
	}
	d->sUid = fn.section('.', 0, 0);
	//qDebug("uid: %s", qPrintable(sUid));
}

QString TomboyNote::title() const
{
	return d->sTitle;
}

QString TomboyNote::uid() const
{
	return d->sUid;
}

QDateTime TomboyNote::modifyTime() const
{
	return d->dtLastChange;
}

QString TomboyNote::text() const
{
	return d->sText;
}

void TomboyNote::setText(const QString &text)
{
	d->sText = text;
	d->sTitle = d->sText.section('\n', 0, 0); //FIXME crossplatform?
}

void TomboyNote::toTrash()
{
	QFile f(d->sFile);
	f.remove();
	//emit
}

void TomboyNote::saveToFile(const QString &fileName)
{
	QDomDocument dom;
	QDomElement note = dom.createElement("note"), node, node2;
	QDomText text;
	dom.appendChild(note);
	note.setAttribute("version", "0.3");
	note.setAttribute("xmlns:link", "http://beatniksoftware.com/tomboy/link");
	note.setAttribute("xmlns:size", "http://beatniksoftware.com/tomboy/size");
	note.setAttribute("xmlns", "http://beatniksoftware.com/tomboy");
	node = dom.createElement("title");
	note.appendChild(node);
	text = dom.createTextNode(d->sTitle);
	node.appendChild(text);
	node = dom.createElement("text");
	note.appendChild(node);
	node.setAttribute("xml:space", "preserve");
	node2 = dom.createElement("note-content");
	node2.setAttribute("version", "0.1");
	text = dom.createTextNode(d->sText);
	node2.appendChild(text);
	node.appendChild(node2);

	setFile(fileName);;
	QFile file(d->sFile);
	file.open(QIODevice::WriteOnly);
	file.write(dom.toString(-1).toUtf8());
	//qDebug()<<sUid<<"\n"<<sFile;
	file.close();
}

QString TomboyNote::nodeText(QDomNode node)
{
	QString ret;
	QDomNode child = node.firstChild();
	while (!child.isNull()) {
		if (child.isText()) {
			ret += child.nodeValue();
		}
		if (child.isElement()) {
			ret += nodeText(child);
		}
		child = child.nextSibling();
	}
	return ret;
}
