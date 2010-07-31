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

#include "tomboydata.h"
#include <QIcon>
#include <QtDebug>

TomboyData::TomboyData()
		: FileNoteData()
{

}

bool TomboyData::fromFile(QString fn)
{
	QDomDocument dom("TomboyData");
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
	sTitle = nodeText(root.namedItem("title"));
	sText = nodeText(root.namedItem("text"));
	dtLastChange = QDateTime::fromString(nodeText(root.namedItem("last-change-date")), Qt::ISODate);
	//dtLastMetadataChange = QDateTime::fromString(nodeText(root.namedItem("last-metadata-change-date")), Qt::ISODate);
	dtCreate = QDateTime::fromString(nodeText(root.namedItem("create-date")), Qt::ISODate);
	iCursor = nodeText(root.namedItem("create-date")).toInt();
	iWidth = nodeText(root.namedItem("width")).toInt();
	iHeight = nodeText(root.namedItem("height")).toInt();

	return true;
}

bool TomboyData::saveToFile(const QString &fileName)
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
	text = dom.createTextNode(sTitle);
	node.appendChild(text);
	node = dom.createElement("text");
	note.appendChild(node);
	node.setAttribute("xml:space", "preserve");
	node2 = dom.createElement("note-content");
	node2.setAttribute("version", "0.1");
	text = dom.createTextNode(sText);
	node2.appendChild(text);
	node.appendChild(node2);

	setFile(fileName);;
	QFile file(sFileName);
	if (!file.open(QIODevice::WriteOnly)) {
		return false;
	}
	file.write(dom.toString(-1).toUtf8());
	//qDebug()<<sUid<<"\n"<<sFile;
	file.close();
	return true;
}

QString TomboyData::nodeText(QDomNode node)
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
