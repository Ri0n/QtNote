#ifndef QTNOTE_STATUSNOTIFIERITEM_H
#define QTNOTE_STATUSNOTIFIERITEM_H

#include <QObject>

namespace QtNote {

class StatusNotifierItemPrivate;

class StatusNotifierItem : public QObject
{
	Q_OBJECT
public:
	explicit StatusNotifierItem(QObject *parent = 0);
	~StatusNotifierItem();

signals:

public slots:

private:
	StatusNotifierItemPrivate *d;
};

} // namespace QtNote

#endif // QTNOTE_STATUSNOTIFIERITEM_H
