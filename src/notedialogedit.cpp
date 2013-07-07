#include <QDropEvent>
#include <QUrl>
#include <QTextCursor>
#include <QMimeData>

#include "notedialogedit.h"


NoteDialogEdit::NoteDialogEdit(QWidget *parent) :
    QTextEdit(parent)
{
}

void NoteDialogEdit::dropEvent(QDropEvent *e)
{
	bool useDefault = true;
	foreach (const QUrl &url, e->mimeData()->urls()) {
		QString fileName = url.toLocalFile();
		if (fileName.isEmpty()) {
			continue;
		}
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly)) {
			continue;
		}
		useDefault = false;
		QByteArray data = file.readAll();
		file.close();
		QString str = QString::fromLocal8Bit(data.constData(), data.size());
		textCursor().insertText(str);
	}
	if (useDefault) {
		QTextEdit::dropEvent(e);
	} else {
		e->accept();
	}
}
