#include <QList>
#include <QSettings>

#include "globalshortcutsinterface.h"
#include "shortcutsmanager.h"

namespace QtNote {

const char *ShortcutsManager::SKNoteFromSelection = "note-from-selection";

struct BaseInfo {
    QString name;
    QString defaultKey;
};

static QHash<QString, BaseInfo> shortcuts;

ShortcutsManager::ShortcutsManager(GlobalShortcutsInterface *gs, QObject *parent) : QObject(parent), gs(gs)
{
    shortcuts.insert(QLatin1String(SKNoteFromSelection), { tr("Note From Selection"), QLatin1String("Ctrl+Alt+M") });
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
    QSettings                           s;
    QString                             opt = QLatin1String("shortcuts.") + option;
    static QHash<QString, QKeySequence> defaults;
    if (!s.contains(opt)) {
        if (defaults.isEmpty()) {
            defaults.insert(QLatin1String(SKNoteFromSelection), QKeySequence("Ctrl+Alt+M"));
        }
        return defaults.value(option);
    }
    return QKeySequence(s.value(opt).toString());
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
    if (globals.contains(option) && gs) {
        return gs->updateGlobalShortcut(option, key(option));
    }
    return true;
}

QList<ShortcutsManager::ShortcutInfo> ShortcutsManager::all() const
{
    QSettings           s;
    QList<ShortcutInfo> ret;
    foreach (const QString &option, shortcuts.keys()) {
        ret.append({ option, shortcuts[option].name, key(option) });
    }
    return ret;
}

QString ShortcutsManager::friendlyName(const QString &option) const { return shortcuts.value(option).name; }

bool ShortcutsManager::registerGlobal(const char *option, QAction *action)
{
    if (gs) {
        QKeySequence ks = key(option);
        if (!ks.isEmpty()) {
            if (gs->registerGlobalShortcut(option, ks, action)) {
                globals.append(QLatin1String(option));
                return true;
            }
        }
    }
    return false;
}

void ShortcutsManager::setShortcutEnable(const QString &option, bool enabled)
{
    if (globals.contains(option) && gs) {
        return gs->setGlobalShortcutEnabled(option, enabled);
    }
}

} // namespace QtNote
