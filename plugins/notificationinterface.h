#ifndef NOTIFICATIONINTERFACE_H
#define NOTIFICATIONINTERFACE_H

#include <QObject>

namespace QtNote {

class Main;

class NotificationInterface
{
public:
	virtual void notifyError(const QString &message) = 0;
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::NotificationInterface,
                    "com.rion-soft.QtNote.NotificationInterface/1.0")

#endif // NOTIFICATIONINTERFACE_H
