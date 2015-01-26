#ifndef HUNSPELLENGINE_H
#define HUNSPELLENGINE_H

#include "engineinterface.h"

class Hunspell;
class QTextCodec;

namespace QtNote {

class HunspellEngine : public SpellEngineInterface
{
public:
	struct LangItem {
		Hunspell *hunspell;
		QTextCodec *codec;
	};

	HunspellEngine();

	bool addLanguage(const QLocale &locale);
	bool spell(const QString &word) const;

private:
	QList<LangItem> languages;
};

} // namespace QtNote

#endif // HUNSPELLENGINE_H
