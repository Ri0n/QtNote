#ifndef TOMBOYNOTE_H
#define TOMBOYNOTE_H

#include <QObject>
#include <QFile>
#include <QDomDocument>
#include <QPointer>
#include <QDateTime>
#include "notedialog.h"

class TomboyNote : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(TomboyNote)
private:
	QPointer<NoteDialog> dlg;
	QWidget *mainWin;

	QString sFile;
	QString sUid;
	QString sTitle;
	QString sText;
	QDateTime dtLastChange;
	QDateTime dtCreate;
	int iCursor;
	int iWidth;
	int iHeight;

public:
	TomboyNote(QWidget *parent = 0);
	TomboyNote(QString, QWidget *parent = 0);
	~TomboyNote();
	bool fromFile(QString);
	void setFile(QString fn);
	void showDialog();
	QString title();

	QString nodeText(QDomNode node);

public slots:
	void onCloseNote(int);
};

#endif // TOMBOYNOTE_H
