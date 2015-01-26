#include "hunspellengine.h"

#include <QString>
#include <QSet>
#include <QDir>
#include <QLibraryInfo>
#include <QTextCodec>
#include <QDebug>
#include <hunspell/hunspell.hxx>

namespace QtNote {

static bool scanDictPaths(const QString &language, QFileInfo &aff , QFileInfo &dic)
{
	QSet<QString> languages;

	QSet<QString> dictPaths;
	QString pathFromEnv = QString::fromLocal8Bit(qgetenv("MYSPELL_DICT_DIR"));
	if (!pathFromEnv.isEmpty())
		dictPaths << pathFromEnv;
#if defined(Q_WS_WIN)
	dictPaths << QCoreApplication::applicationDirPath() + QLatin1String("/myspell/dicts");
	dictPaths << QLibraryInfo::location(QLibraryInfo::DataPath) + QLatin1String("/myspell/dicts");
#elif defined(Q_OS_MAC)
	dictPaths << QLatin1String("/opt/local/share/myspell"); // MacPorts standard paths
#else
	dictPaths << QLatin1String("/usr/share/myspell")
			  << QLatin1String("/usr/local/share/myspell");
#endif

	foreach (const QString &dictPath, dictPaths.toList()) {
		QDir dir(dictPath);
		if (dir.exists()) {
			QFileInfo affInfo(dir.filePath(language + QLatin1String(".aff")));
			QFileInfo dicInfo(dir.filePath(language + QLatin1String(".dic")));
			if (affInfo.isReadable() && dicInfo.isReadable()) {
				aff = affInfo;
				dic = dicInfo;
				return true;
			}
		}
	}

	return false;
}

HunspellEngine::HunspellEngine()
{

}

bool HunspellEngine::addLanguage(const QLocale &locale)
{
	QString language = locale.name();
	QFileInfo aff, dic;
	if (scanDictPaths(language, aff, dic)) {
		LangItem li;
		qDebug() << "Add hunspell:" << aff.absoluteFilePath() << dic.absoluteFilePath();
		li.hunspell = new Hunspell(aff.absoluteFilePath().toLocal8Bit(),
						  dic.absoluteFilePath().toLocal8Bit());
		li.codec = QTextCodec::codecForName(QByteArray(li.hunspell->get_dic_encoding()));
		li.info.language = locale.language();
		li.info.country = locale.country();
		li.info.filename = dic.filePath();
		languages.append(li);
		return true;
	}
	return false;
}

bool HunspellEngine::spell(const QString &word) const
{
	foreach (const LangItem &li, languages) {
		if (li.hunspell->spell(li.codec->fromUnicode(word)) != 0) {
			return true;
		}
	}
	return false;
}

QList<SpellEngineInterface::DictInfo> HunspellEngine::loadedDicts() const
{
	QList<DictInfo> ret;
	foreach (const LangItem &li, languages) {
		ret.append(li.info);
	}
	return ret;
}

}
