#ifndef SHORTCUTSMANAGER_H
#define SHORTCUTSMANAGER_H

#include <QObject>
#include <QHash>

class QAction;
class QKeySequence;
class QxtGlobalShortcut;

class ShortcutsManager : public QObject
{
	Q_OBJECT

	class ShortcutAction
	{
	public:
		ShortcutAction() : action(0), shortcut(0) {}

		QAction *action;
		QxtGlobalShortcut *shortcut;
		QString option;
	};

public:
	explicit ShortcutsManager(QObject *parent = 0);
	QAction *shortcut(const QLatin1String &option);
	bool setShortcutSeq(const QString &option, const QKeySequence &seq);
	
signals:
	
public slots:

private:
	QHash<QString, ShortcutAction> shortcuts;

	bool updateShortcut(ShortcutAction &sa);
};

#endif // SHORTCUTSMANAGER_H
