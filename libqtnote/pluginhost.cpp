#include "pluginhost.h"
#include "notehighlighter.h"
#include "notemanager.h"
#include "notewidget.h"
#include "spellcheckprovider.h"
#include "utils.h"

namespace QtNote {

PluginHost::PluginHost(QObject *parent) : QObject(parent) { }

QString PluginHost::utilsCuttedDots(const QString &str, int n) { return Utils::cuttedDots(str, n); }

NoteManager *PluginHost::noteManager() { return NoteManager::instance(); }

QString PluginHost::qtnoteDataDir() { return Utils::qtnoteDataDir(); }

void PluginHost::rehighlight() { emit rehightlight_requested(); }

void PluginHost::addHighlightExtension(QWidget *w, std::shared_ptr<HighlighterExtension> ext, int type)
{
    qobject_cast<NoteWidget *>(w)->addHighlightExtension(ext, type);
}

bool PluginHost::offerSpellCheckProvider(std::shared_ptr<SpellCheckProvider> provider)
{
    if (!provider || !provider->isValid())
        return false;
    if (provider_) {
        if (provider_->id() != provider->id()) {
            qWarning() << "Spell check provider ignored:" << provider->id() << "active provider:" << provider_->id();
            provider->disable(
                SpellCheckProvider::DisableMode::Session,
                tr("Not initialized because %1 is the active spell checker.").arg(provider_->displayName()));
            const QString signature = provider_->id() + QLatin1Char('|') + provider->id();
            if (!notifiedSpellCheckConflicts_.contains(signature)) {
                notifiedSpellCheckConflicts_.insert(signature);
                emit spellCheckProviderConflict(provider_->displayName(), provider->displayName());
            }
        }
        return provider_->id() == provider->id();
    }

    auto extension = makeSpellCheckExtension(provider);
    if (!extension)
        return false;
    provider_            = std::move(provider);
    spellCheckExtension_ = std::move(extension);
    qInfo() << "Spell check provider selected:" << provider_->id();
    return true;
}

void PluginHost::attachSpellCheck(QWidget *w)
{
    if (spellCheckExtension_)
        addHighlightExtension(w, spellCheckExtension_, int(NoteHighlighter::SpellCheck));
}

QString PluginHost::activeSpellCheckProviderId() const { return provider_ ? provider_->id() : QString(); }

} // namespace QtNote
