#ifndef GLOBALSHORTCUTSINTERFACE_H
#define GLOBALSHORTCUTSINTERFACE_H

class QKeySequence;
class QObject;
class QString;

namespace QtNote {

class GlobalShortcutsInterface
{
public:
    // TODO it's better to use QAction here instead of receiver/slot
    virtual bool registerGlobalShortcut(const QString &id, const QKeySequence &key, QObject *receiver, const char *slot) = 0;
    virtual bool updateGlobalShortcut(const QString &id, const QKeySequence &key) = 0;
    // TODO unregister. add string identifier
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::GlobalShortcutsInterface,
                    "com.rion-soft.QtNote.GlobalShortcutsInterface/1.1")


#endif // GLOBALSHORTCUTSINTERFACE_H
