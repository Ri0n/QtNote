#include <QSettings>

#include "qxtglobalshortcut.h"
#include "shortcutsmanager.h"

ShortcutsManager::ShortcutsManager(QObject *parent) :
	QObject(parent)
{
}

bool ShortcutsManager::initShortcut(const QLatin1String &name, QObject *object, const char *slot)
{
	QSettings s;
	QKeySequence seq(s.value("shortcut." + name).toString());
	QxtGlobalShortcut *shortcut = new QxtGlobalShortcut(object);
	connect(shortcut, SIGNAL(activated()), object, slot);
	shortcuts.insert(name, shortcut);
	return setShortcutSeq(name, seq);
}

bool ShortcutsManager::updateShortcut(const QLatin1String &name, const QKeySequence &seq)
{
	QxtGlobalShortcut *shortcut = shortcuts.value(name);
	if (shortcut) {
		return setShortcutSeq(shortcut, seq);
	}
	return false;
}

bool ShortcutsManager::setShortcutSeq(QxtGlobalShortcut *shortcut, const QKeySequence &seq)
{

}
