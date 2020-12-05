#include "pluginhost.h"
#include "noteedit.h"
#include "notehighlighter.h"
#include "notemanager.h"
#include "notewidget.h"
#include "utils.h"

namespace QtNote {

PluginHost::PluginHost(QObject *parent) : QObject(parent) { }

QString PluginHost::utilsCuttedDots(const QString &str, int n) { return Utils::cuttedDots(str, n); }

NoteManager *PluginHost::noteManager() { return NoteManager::instance(); }

QString PluginHost::qtnoteDataDir() { return Utils::qtnoteDataDir(); }

void PluginHost::rehighlight() { emit rehightlight_requested(); }

NoteEdit *PluginHost::noteTextWidget(QWidget *w) { return qobject_cast<NoteWidget *>(w)->editWidget(); }

void PluginHost::addHighlightExtension(QWidget *w, std::shared_ptr<HighlighterExtension> ext, int type)
{
    qobject_cast<NoteWidget *>(w)->highlighter()->addExtension(ext, NoteHighlighter::ExtType(type));
}

} // namespace QtNote
