#include "tomboynote.h"
#include <QIcon>

TomboyNote::TomboyNote(QWidget *parent)
		: dlg(0), mainWin(parent)
{
	sTitle = "(no name)";
}

TomboyNote::TomboyNote(QString file, QWidget *parent)
		: dlg(0), mainWin(parent)
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
	file.close();

	QDomNode child = dom.documentElement().firstChild();
	while (!child.isNull()) {
		QString nodeName = child.nodeName();
		if (nodeName == "title") {
			sTitle = nodeText(child);
		} else if (nodeName == "text") {
			sText = nodeText(child);
		} else {

/*			case "title":
				sTitle = child.firstChild().nodeValue();
				break;
			case "text":
				sText = child.firstChild().nodeValue();
				break;
			case "last-change-date":
				break;
			case "last-metadata-change-date":
				break;
			case "create-date":
				break;
			case "cursor-position":
				break;
			case "width":
				break;
			case "height":
				break;
			case "x":
				break;
			case "y":
				break;
			case "tags":
				break;
			case "open-on-startup":
				break;*/
		}
		child = child.nextSibling();
	}
	return true;
}

void TomboyNote::showDialog()
{
	if (!dlg) {
		dlg = new NoteDialog(mainWin);
		dlg->setText(sText);
		dlg->setWindowIcon(QIcon(":/icons/pen"));
		dlg->setWindowTitle(sTitle);
		dlg->show();
		connect(dlg, SIGNAL(finished(int)), this, SLOT(onCloseNote(int)));
	}
	dlg->raise();
	dlg->activateWindow();
}

QString TomboyNote::title()
{
	return sTitle;
}

void TomboyNote::onCloseNote(int result)
{
	sText = dlg->text();
}

QString TomboyNote::nodeText(QDomNode &node)
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
