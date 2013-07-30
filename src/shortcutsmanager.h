#ifndef SHORTCUTSMANAGER_H
#define SHORTCUTSMANAGER_H

#include <QObject>
#include <QHash>
#include <QKeySequence>

class QAction;
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
	QMap<QString, QString> &optionsMap() const;
	QAction *shortcut(const QLatin1String &option);
	QKeySequence sequence(const QString &option) const;
	bool setShortcutSeq(const QString &option, const QKeySequence &seq);
	
signals:
	
public slots:

private:
	QHash<QString, ShortcutAction> shortcuts;

	bool updateShortcut(ShortcutAction &sa);
};

#endif // SHORTCUTSMANAGER_H
