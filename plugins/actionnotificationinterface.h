#ifndef ACTIONNOTIFICATIONINTERFACE_H
#define ACTIONNOTIFICATIONINTERFACE_H

#include <QString>
#include <functional>

namespace QtNote {

class ActionNotificationInterface {
public:
    virtual ~ActionNotificationInterface() = default;
    virtual void notify(const QString &title, const QString &message, const QString &actionText,
                        std::function<void()> action)
        = 0;
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::ActionNotificationInterface, "com.rion-soft.QtNote.ActionNotificationInterface/1.0")

#endif // ACTIONNOTIFICATIONINTERFACE_H
