#include <QSettings>
#include <QAction>

#include "qxtglobalshortcut.h"
#include "shortcutsmanager.h"

ShortcutsManager::ShortcutsManager(QObject *parent) :
	QObject(parent)
{
}

const QMap<QString, QString> &ShortcutsManager::optionsMap() const
{
	static QMap<QString, QString> map;
	if (map.isEmpty()) {
		map.insert(QLatin1String("note-from-selection"), tr("Note From Selection"));
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

QKeySequence ShortcutsManager::sequence(const QString &option) const
{
	QSettings s;
	return QKeySequence(s.value("shortcuts." + option).toString());
}

bool ShortcutsManager::updateShortcut(ShortcutAction &sa)
{
	QSettings s;
	QKeySequence seq(s.value("shortcuts." + sa.option).toString());
	if (sa.shortcut && sa.shortcut->shortcut() == seq) {
		return true;
	}
	delete sa.shortcut;
	sa.shortcut = new QxtGlobalShortcut(this);
	connect(sa.shortcut, SIGNAL(activated()), sa.action, SIGNAL(triggered()));
	return sa.shortcut->setShortcut(seq);
}

bool ShortcutsManager::setShortcutSeq(const QString &option, const QKeySequence &seq)
{
	QSettings s;
	ShortcutAction &sa = shortcuts[option];
	s.setValue("shortcuts." + option, seq.toString());
	return updateShortcut(sa);
}
