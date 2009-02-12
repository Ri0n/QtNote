#ifndef TOMBOYNOTE_H
#define TOMBOYNOTE_H

#include <QObject>
#include <QFile>
#include <QDomDocument>
#include <QPointer>
#include "notedialog.h"

class TomboyNote : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(TomboyNote)
private:
	QPointer<NoteDialog> dlg;
	QWidget *mainWin;

	QString sTitle;
	QString sText;

public:
	TomboyNote(QWidget *parent = 0);
	TomboyNote(QString, QWidget *parent = 0);
	~TomboyNote();
	bool fromFile(QString);
	void showDialog();
	QString title();

	QString nodeText(QDomNode &node);

public slots:
	void onCloseNote(int);
};

#endif // TOMBOYNOTE_H
