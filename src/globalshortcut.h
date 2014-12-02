#ifndef GLOBALSHORTCUT_H
#define GLOBALSHORTCUT_H

#include <QObject>

class QKeySequence;

namespace QtNote {

class GlobalShortcut : public QObject
{
	Q_OBJECT

public:

protected:
	virtual void registerShortcut(const QKeySequence &key, const QString &id, const QString &humanName) = 0;

signals:
	void triggered();
};

}

#endif // GLOBALSHORTCUT_H
