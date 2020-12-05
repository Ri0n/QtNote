#ifndef HUNSPELLENGINE_H
#define HUNSPELLENGINE_H

#include <QSet>

#include "engineinterface.h"

class Hunspell;
class QTextCodec;

namespace QtNote {

class PluginHostInterface;

class HunspellEngine : public SpellEngineInterface {
public:
    struct LangItem {
        DictInfo    info;
        Hunspell *  hunspell;
        QTextCodec *codec;
    };

    HunspellEngine(PluginHostInterface *host);
    ~HunspellEngine();

    QList<QLocale>  supportedLanguages() const;
    bool            addLanguage(const QLocale &locale);
    bool            spell(const QString &word) const;
    void            addToDictionary(const QString &word);
    QList<QString>  suggestions(const QString &word);
    QList<DictInfo> loadedDicts() const;

private:
    PluginHostInterface *host;
    QList<LangItem>      languages;
    QSet<QString>        runtimeDict;
};

} // namespace QtNote

#endif // HUNSPELLENGINE_H
