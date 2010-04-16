#include "tomboynote.h"
#include <QIcon>
#include <QtDebug>

TomboyNote::TomboyNote(QWidget *parent)
		: Note(parent)
{
	sTitle = "(no name)";
}

TomboyNote::TomboyNote(QString file, QWidget *parent)
		: Note(parent)
{
	fromFile(file);
}

TomboyNote::~TomboyNote()
{
	if (dlg) {
		delete dlg;
	}
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

void TomboyNote::setFile(QString fn)
{
	sFile = fn;
	int pos = fn.lastIndexOf('/');
	if (pos != -1) {
		fn = fn.mid(pos+1);
	}
	sUid = fn.section('.', 0, 0);
}

QString TomboyNote::title() const
{
	return sTitle;
}

QString TomboyNote::uid() const
{
	return sUid;
}

QDateTime TomboyNote::modifyTime() const
{
	return dtLastChange;
}

QString TomboyNote::text() const
{
	return sText;
}

void TomboyNote::toTrash()
{
	QFile f(sFile);
	f.remove();
	//emit
}

void TomboyNote::onCloseNote(int result)
{
	if (result == QDialog::Rejected) {
		return;
	}
	sText = dlg->text();
	sTitle = sText.section('\n', 0, 0); //FIXME crossplatform?
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

	QFile file(sFile);
	file.open(QIODevice::WriteOnly);
	file.write(dom.toString(-1).toUtf8());
	qDebug()<<sUid<<"\n"<<sFile;
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
