#ifndef GLOBALSHORTCUTSINTERFACE_H
#define GLOBALSHORTCUTSINTERFACE_H

class QKeySequence;
class QObject;

namespace QtNote {

class GlobalShortcutsInterface
{
public:
	virtual void registerGlobalShortcut(const QKeySequence &key, QObject *receiver, const char *slot) = 0;
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::GlobalShortcutsInterface,
					 "com.rion-soft.QtNote.GlobalShortcutsInterface/1.0")


#endif // GLOBALSHORTCUTSINTERFACE_H
