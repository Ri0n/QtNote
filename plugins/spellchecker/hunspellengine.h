#ifndef HUNSPELLENGINE_H
#define HUNSPELLENGINE_H

#include <QSet>

#include "engineinterface.h"

class Hunspell;
class QTextCodec;
class QStringEncoder;
class QStringDecoder;

namespace QtNote {

class PluginHostInterface;

class HunspellEngine : public SpellEngineInterface {
public:
    struct LangItem {
        DictInfo  info;
        Hunspell *hunspell;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QTextCodec *codec;
#else
        QStringEncoder *encoder = nullptr;
        QStringDecoder *decoder = nullptr;
#endif
    };

    HunspellEngine(PluginHostInterface *host);
    ~HunspellEngine();

    QList<QLocale>  supportedLanguages() const override;
    bool            addLanguage(const QLocale &locale) override;
    bool            spell(const QString &word) const override;
    void            addToDictionary(const QString &word) override;
    QList<QString>  suggestions(const QString &word) const override;
    QList<DictInfo> loadedDicts() const override;

    QStringList     diagnostics() const override;

private:
    PluginHostInterface *host;
    QList<LangItem>      languages;
    QSet<QString>        runtimeDict;
    QStringList          dictPaths;
};

} // namespace QtNote

#endif // HUNSPELLENGINE_H
