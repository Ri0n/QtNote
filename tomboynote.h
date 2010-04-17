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
	TomboyNote(QObject *parent = 0);
	TomboyNote(QString, QWidget *parent = 0);
	~TomboyNote();
	bool fromFile(QString);
	void setFile(QString fn);
	void saveToFile(const QString &fileName);
	QString title() const;
	QString uid() const;
	QString text() const;
	void setText(const QString &text);
	QDateTime modifyTime() const;
	void toTrash();

	QString nodeText(QDomNode node);
};

#endif // TOMBOYNOTE_H
