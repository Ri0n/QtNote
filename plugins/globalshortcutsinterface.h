#ifndef GLOBALSHORTCUTSINTERFACE_H
#define GLOBALSHORTCUTSINTERFACE_H

class QKeySequence;
class QObject;
class QString;

namespace QtNote {

class GlobalShortcutsInterface
{
public:
	virtual bool registerGlobalShortcut(const QString &id, const QKeySequence &key, QObject *receiver, const char *slot) = 0;
	// TODO unregister. add string identifier
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::GlobalShortcutsInterface,
					 "com.rion-soft.QtNote.GlobalShortcutsInterface/1.0")


#endif // GLOBALSHORTCUTSINTERFACE_H
