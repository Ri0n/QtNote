#ifndef NOTE_H
#define NOTE_H

#include <QObject>

class Note : public QObject
{
	Q_OBJECT
public:
	Note(QObject *parent);

	virtual void toTrash() = 0;
	virtual QString text() const = 0;
	virtual QString title() const = 0;
};

#endif // NOTE_H
