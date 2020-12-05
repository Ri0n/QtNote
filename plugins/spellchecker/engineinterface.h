#ifndef ENGINEINTERFACE_H
#define ENGINEINTERFACE_H

#include <QLocale>

namespace QtNote {

class SpellEngineInterface {
public:
    struct DictInfo {
        QLocale::Language language;
        QLocale::Country  country;
        QString           filename;
    };

    virtual ~SpellEngineInterface() { }

    virtual QList<QLocale> supportedLanguages() const         = 0;
    virtual bool           addLanguage(const QLocale &locale) = 0;

    // true - good word, false - bad word
    virtual bool            spell(const QString &word) const     = 0;
    virtual void            addToDictionary(const QString &word) = 0;
    virtual QList<QString>  suggestions(const QString &word)     = 0;
    virtual QList<DictInfo> loadedDicts() const                  = 0;
};

} // namespace QtNote

#endif // ENGINEINTERFACE_H
