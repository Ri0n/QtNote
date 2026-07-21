#ifndef QTNOTE_PLUGINHOST_H
#define QTNOTE_PLUGINHOST_H

#include "pluginhostinterface.h"

#include <QObject>
#include <QSet>

namespace QtNote {

class PluginHost : public QObject, public PluginHostInterface {
    Q_OBJECT
public:
    explicit PluginHost(QObject *parent = nullptr);
    QString      utilsCuttedDots(const QString &str, int n) override;
    NoteManager *noteManager() override;
    QString      qtnoteDataDir() override;
    void         rehighlight() override;
    void         addHighlightExtension(QWidget *w, std::shared_ptr<HighlighterExtension> ext, int type) override;
    bool         offerSpellCheckProvider(std::shared_ptr<SpellCheckProvider> provider) override;
    void         attachSpellCheck(QWidget *w);

    QString activeSpellCheckProviderId() const;

signals:
    void rehightlight_requested();
    void spellCheckProviderConflict(const QString &activeName, const QString &ignoredName);

private:
    std::shared_ptr<SpellCheckProvider>   provider_;
    std::shared_ptr<HighlighterExtension> spellCheckExtension_;
    QSet<QString>                         notifiedSpellCheckConflicts_;
};

} // namespace QtNote

#endif // QTNOTE_PLUGINHOST_H
