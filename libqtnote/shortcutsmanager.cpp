#include <QSettings>
#include <QList>

#include "shortcutsmanager.h"
#include "globalshortcutsinterface.h"

namespace QtNote {

const char* ShortcutsManager::SKNoteFromSelection = "note-from-selection";

static QHash<QString, QString> shortcuts;

ShortcutsManager::ShortcutsManager(GlobalShortcutsInterface *gs, QObject *parent) :
	QObject(parent),
	gs(gs)
{
	shortcuts.insert(QLatin1String(SKNoteFromSelection), tr("Note From Selection"));
}

#if 0
const QMap<QString, QString> &ShortcutsManager::optionsMap() const
{
	static QMap<QString, QString> map;
	if (map.isEmpty()) {
		map.insert(QLatin1String(SKNoteFromSelection), tr("Note From Selection"));
	}
	return map;
}

QAction* ShortcutsManager::shortcut(const QLatin1String &option)
{
	ShortcutAction &sa = shortcuts[option];
	if (!sa.action) {
		sa.action = new QAction(this);
		sa.option = option;
		updateShortcut(sa);
	}
	return sa.action;
}
#endif
QKeySequence ShortcutsManager::key(const QString &option) const
{
	QSettings s;
	return QKeySequence(s.value("shortcuts." + option).toString());
}

#if 0
bool ShortcutsManager::updateShortcut(ShortcutAction &sa)
{
	QSettings s;
	QKeySequence seq(s.value("shortcuts." + sa.option).toString());
	if (sa.shortcut.key() == seq) {
		return true;
	}
	/*delete sa.shortcut;
	sa.shortcut = new QxtGlobalShortcut(this);
	connect(sa.shortcut, SIGNAL(activated()), sa.action, SIGNAL(triggered()));*/
	return sa.shortcut.setKey(seq);
}
#endif

bool ShortcutsManager::setKey(const QString &option, const QKeySequence &seq)
{
	QSettings s;
	s.setValue("shortcuts." + option, seq.toString());
	// TODO check if it's global and reassign
	return true;
}

QList<ShortcutsManager::ShortcutInfo> ShortcutsManager::all() const
{
	QSettings s;
	QList<ShortcutInfo> ret;
	foreach (const QString &option, shortcuts.keys()) {
		ret.append({option, shortcuts[option], s.value("shortcuts." + option).toString()});
	}
	return ret;
}

QString ShortcutsManager::friendlyName(const QString &option) const
{
	return shortcuts.value(option);
}

bool ShortcutsManager::registerGlobal(const char *option, QObject *receiver, const char *slot)
{
	if (gs) {
		QKeySequence ks = key(option);
		if (!ks.isEmpty()) {
			return gs->registerGlobalShortcut(option, ks, receiver, slot);
		}
	}
	return false;
}

} // namespace QtNote

