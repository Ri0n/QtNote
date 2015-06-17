#include "hunspellengine.h"

#include <QString>
#include <QSet>
#include <QDir>
#include <QLibraryInfo>
#include <QTextCodec>
#include <QCoreApplication>
#include <hunspell/hunspell.hxx>

namespace QtNote {

static QStringList dictPaths()
{
	static QStringList dictPaths;
	if (dictPaths.isEmpty()) {
		QSet<QString> dictPathSet;
		QString pathFromEnv = QString::fromLocal8Bit(qgetenv("MYSPELL_DICT_DIR"));
		if (!pathFromEnv.isEmpty())
			dictPathSet << pathFromEnv;
#if defined(Q_OS_WIN)
		dictPathSet << QCoreApplication::applicationDirPath() + QLatin1String("/myspell/dicts");
		dictPathSet << QLibraryInfo::location(QLibraryInfo::DataPath) + QLatin1String("/myspell/dicts");
#elif defined(Q_OS_MAC)
		dictPathSet << QLatin1String("/opt/local/share/myspell"); // MacPorts standard paths
#else
		dictPathSet << QLatin1String("/usr/share/myspell")
				  << QLatin1String("/usr/share/hunspell")
				  << QLatin1String("/usr/local/share/myspell")
				  << QLatin1String("/usr/local/share/hunspell");
#endif
		dictPaths = dictPathSet.toList();
	}
	return dictPaths;
}

static bool scanDictPaths(const QString &language, QFileInfo &aff , QFileInfo &dic)
{
	foreach (const QString &dictPath, dictPaths()) {
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

QList<QLocale> HunspellEngine::supportedLanguages() const
{
	QMap<QString,QLocale> retHash;
	foreach (const QString &dictPath, dictPaths()) {
		QDir dir(dictPath);
		if (!dir.exists()) {
			continue;
		}
		foreach (const QFileInfo &fi, dir.entryInfoList(QStringList() << "*.dic", QDir::Files)) {
			QLocale locale(fi.baseName());
			if (locale != QLocale::c())  {
				retHash.insert(locale.nativeLanguageName()+locale.nativeCountryName(), locale);
			}
		}
	}
	return retHash.values();
}

bool HunspellEngine::addLanguage(const QLocale &locale)
{
	QString language = locale.name();
	QFileInfo aff, dic;
	if (scanDictPaths(language, aff, dic)) {
		LangItem li;
		//qDebug() << "Add hunspell:" << aff.absoluteFilePath() << dic.absoluteFilePath();
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
