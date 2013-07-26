#ifndef SHORTCUTSMANAGER_H
#define SHORTCUTSMANAGER_H

#include <QObject>
#include <QHash>

class QAction;
class QxtGlobalShortcut;

class ShortcutsManager : public QObject
{
	Q_OBJECT
public:
	explicit ShortcutsManager(QObject *parent = 0);
	bool initShortcut(const QLatin1String &name, QObject *object, const char *slot);
	bool updateShortcut(const QLatin1String &name, const QKeySequence &seq);
	
signals:
	
public slots:

private:
	QHash<QString, QxtGlobalShortcut*> shortcuts;
	
	bool setShortcutSeq(QxtGlobalShortcut *shortcut, const QKeySequence &seq);
};

#endif // SHORTCUTSMANAGER_H
