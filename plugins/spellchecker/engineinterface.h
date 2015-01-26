#ifndef ENGINEINTERFACE_H
#define ENGINEINTERFACE_H

#include <QLocale>

namespace QtNote {

class SpellEngineInterface
{
public:
	virtual bool addLanguage(const QLocale &locale) = 0;
	virtual bool spell(const QString &word) const = 0;
};

} // namespace QtNote

#endif // ENGINEINTERFACE_H
