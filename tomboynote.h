#ifndef TOMBOYNOTE_H
#define TOMBOYNOTE_H

#include <QObject>
#include <QFile>
#include <QDomDocument>
#include <QPointer>
#include <QDateTime>
#include "notedialog.h"
#include "note.h"

class TomboyNote : public Note
{
	Q_OBJECT
	Q_DISABLE_COPY(TomboyNote)
private:
	QPointer<NoteDialog> dlg;

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
	QString title() const;
	QString uid() const;
	QString text() const;
	QDateTime modifyTime() const;
	void toTrash();

	QString nodeText(QDomNode node);

public slots:
	void onCloseNote(int);
};

#endif // TOMBOYNOTE_H
