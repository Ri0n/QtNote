#include "hunspellengine.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QLibraryInfo>
#include <QSet>
#include <QStandardPaths>
#include <QString>
#include <QNetworkAccessManager>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#else
#include <QStringDecoder>
#include <QStringEncoder>
#endif
#ifdef Q_OS_WIN
#include <hunspell/hunspell.hxx>
#else
#include <hunspell.hxx>
#endif

#include "hunspelldownloader.h"
#include "pluginhostinterface.h"
#include "qtnote_config.h"
#include "utils.h"

namespace QtNote {

namespace {
    QStringList diagnostics;
    void        addDiag(QString &&s)
    {
        diagnostics.append(std::move(s));
        if (diagnostics.size() > 50)
            diagnostics.pop_front();
    }
}

static QStringList findDictPaths()
{
    QStringList dictPaths;
#if defined(Q_OS_WIN)
    dictPaths << QCoreApplication::applicationDirPath() + QLatin1String(stringify(DICTSUBDIR));
    dictPaths << Utils::qtnoteDataDir() + QLatin1String(stringify(DICTSUBDIR));
#elif defined(Q_OS_MAC)
    dictPaths << QLatin1String("/opt/local/share/myspell"); // MacPorts standard paths
#else
    dictPaths << QLatin1String("/usr/share/myspell") << QLatin1String("/usr/share/hunspell")
              << QLatin1String("/usr/local/share/myspell") << QLatin1String("/usr/local/share/hunspell");
#endif
    dictPaths += QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("hunspell"),
                                           QStandardPaths::LocateDirectory);
    dictPaths += QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("myspell"),
                                           QStandardPaths::LocateDirectory);

    QSet<QString> uniqueDirs;
    uniqueDirs.reserve(dictPaths.size());
    for (auto it = dictPaths.begin(); it < dictPaths.end(); ++it) {
        auto fi = QFileInfo(*it);
        if (fi.isDir() && fi.isReadable()) {
            uniqueDirs << QFileInfo(*it).canonicalFilePath();
        } else {
            diagnostics.append(QObject::tr("%1 is not readable").arg(fi.absoluteFilePath()));
        }
    }
    dictPaths = QStringList(uniqueDirs.begin(), uniqueDirs.end());
    std::sort(dictPaths.begin(), dictPaths.end(), [](const auto &a, const auto &b) {
        auto aw = QFileInfo(a).isWritable();
        auto bw = QFileInfo(b).isWritable();
        if (aw == bw) {
            static auto local = QStringLiteral("local");
            auto        al    = a.contains(local);
            auto        bl    = b.contains(local);
            if (al == bl) {
                return a.size() > b.size();
            }
            return al;
        }
        return aw;
    });
    QString pathFromEnv = QString::fromLocal8Bit(qgetenv("MYSPELL_DICT_DIR"));
    if (!pathFromEnv.isEmpty()) {
        auto cfp = QFileInfo(pathFromEnv).canonicalFilePath();
        if (!uniqueDirs.contains(cfp))
            dictPaths.prepend(cfp);
    }

    return dictPaths;
}

static bool scanDictPaths(const QStringList &dictPaths, const QString &language, QFileInfo &aff, QFileInfo &dic)
{
    foreach (const QString &dictPath, dictPaths) {
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

HunspellEngine::HunspellEngine(PluginHostInterface *host) : host(host)
{
    QFile f(host->qtnoteDataDir() + QLatin1String("/spellcheck-custom.words"));
    if (f.open(QIODevice::ReadOnly)) {
        QDataStream in(&f);
        QString     w;
        while (!in.atEnd()) {
            in >> w;
            runtimeDict << w;
        }
    }
    dictPaths = findDictPaths();
}

HunspellEngine::~HunspellEngine()
{
    delete qnam;
    foreach (const LangItem &li, languages) {
        delete li.hunspell;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        delete li.encoder;
        delete li.decoder;
#endif
    }
    QFile f(host->qtnoteDataDir() + QLatin1String("/spellcheck-custom.words"));
    if (f.open(QIODevice::WriteOnly)) {
        QDataStream out(&f);
        for (auto &w : std::as_const(runtimeDict)) {
            out << w;
        }
    } else {
        qDebug("Failed to write runtime spellcheck dictionary");
    }
}

QList<HunspellEngine::DictInfo> HunspellEngine::supportedLanguages() const
{
    QMap<QString, DictInfo> retHash;
    foreach (const QString &dictPath, dictPaths) {
        QDir dir(dictPath);
        if (!dir.exists()) {
            addDiag(QObject::tr("Directory %1 doesn't exist").arg(dictPath));
            continue;
        }
        addDiag(QObject::tr("Checking if %1 has dictionaries").arg(dictPath));
        foreach (const QFileInfo &fi, dir.entryInfoList(QStringList() << "*.dic", QDir::Files)) {
            QLocale locale(fi.baseName());
            if (locale != QLocale::c()) {
                DictInfo info;
                info.filename = fi.filePath();
                info.language = locale.language();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                info.country = locale.country();
                QString key  = locale.nativeLanguageName() + locale.nativeCountryName();
#else
                info.country = locale.territory();
                QString key  = locale.nativeLanguageName() + locale.nativeTerritoryName();
#endif
                info.isLoaded   = std::ranges::any_of(languages, [&info](auto const &l) {
                    return l.info.country == info.country && l.info.language == info.language;
                });
                info.isWritable = fi.isWritable();
                retHash.insert(key, info);
                addDiag(QObject::tr("Found %1 dictionary").arg(fi.baseName()));
            } else {
                addDiag(QObject::tr("Ignore %1 dictionary as C locale").arg(fi.baseName()));
            }
        }
    }
    return retHash.values();
}

HunspellEngine::Error HunspellEngine::addLanguage(const QLocale &locale)
{
    if (findLangItem(locale) != -1) {
        return NoError; // already added
    }
    QString   language = locale.name();
    QFileInfo aff, dic;
    if (scanDictPaths(dictPaths, language, aff, dic)) {
        LangItem li;
        // qDebug() << "Add hunspell:" << aff.absoluteFilePath() << dic.absoluteFilePath();
        li.hunspell = new Hunspell(aff.absoluteFilePath().toLocal8Bit(), dic.absoluteFilePath().toLocal8Bit());
        QByteArray codecName(li.hunspell->get_dic_encoding());
        if (codecName.startsWith("microsoft-cp125")) {
            codecName.replace(0, sizeof("microsoft-cp") - 1, "Windows-");
        } else if (codecName.startsWith("TIS620-2533")) {
            codecName.resize(sizeof("TIS620") - 1);
        }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        li.codec = QTextCodec::codecForName(codecName);
        if (li.codec) {
#else
        li.encoder = new QStringEncoder(codecName.data());
        li.decoder = new QStringDecoder(codecName.data());
        if (li.encoder->isValid()) {
#endif
            li.info.language = locale.language();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            li.info.country = locale.country();
#else
            li.info.country = locale.territory();
#endif
            li.info.filename = dic.filePath();
            languages.append(li);
            return NoError;
        }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        delete li.encoder;
        delete li.decoder;
#endif
        delete li.hunspell;
        lastError_ = QString("Unsupported myspell dict encoding: \"%1\" for %2").arg(codecName, dic.fileName());
        qWarning("%s", qUtf8Printable(lastError_));
        return InitError;
    }
    return NotInstalledError;
}

void HunspellEngine::removeLanguage(const QLocale &locale)
{
    int index = findLangItem(locale);
    if (index != -1) {
        delete languages[index].hunspell;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        delete languages[index].encoder;
        delete languages[index].decoder;
#endif
        languages.removeAt(index);
    }
}

bool HunspellEngine::canDownload(const QLocale &) const { return true; /* TODO check cache if it's already there */ }

DictionaryDownloader *HunspellEngine::download(const QLocale &locale)
{
    if (!qnam) {
        qnam = new QNetworkAccessManager(qApp);
    }
    auto storePath  = Utils::qtnoteDataDir() + QLatin1String(stringify(DICTSUBDIR));
    auto downloader = new HunspellDownloader(locale, storePath, qnam, this);
    connect(downloader, &HunspellDownloader::finished, this, [downloader, this]() {
        if (!downloader->hasErrors()) {
            dictPaths = findDictPaths(); // downloader could create a dir
            emit availableDictsUpdated();
        }
    });
    downloader->setAutoDelete(true);
    downloader->start();
    return downloader;
}

void HunspellEngine::removeDictionary(const QLocale &locale)
{
    removeLanguage(locale);
    QString   language = locale.name();
    QFileInfo aff, dic;
    if (scanDictPaths(dictPaths, language, aff, dic)) {
        QFile::remove(aff.filePath());
        QFile::remove(dic.filePath());
    }
}

bool HunspellEngine::spell(const QString &word) const
{
    if (runtimeDict.size() && runtimeDict.contains(word)) {
        return true;
    }
    foreach (const LangItem &li, languages) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        auto ba = li.codec->fromUnicode(word); // byte array in dict's encoding
#else
        QByteArray ba = li.encoder->encode(word);
#endif
        if (li.hunspell->spell(std::string(ba.data(), size_t(ba.size()))) != 0) {
            return true;
        }
    }
    return false;
}

void HunspellEngine::addToDictionary(const QString &word) { runtimeDict.insert(word); }

QList<QString> HunspellEngine::suggestions(const QString &word) const
{
    QStringList qtResult;
    foreach (const LangItem &li, languages) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        auto result = li.hunspell->suggest(std::string(li.codec->fromUnicode(word)));
#else
        auto result = li.hunspell->suggest(std::string(QByteArray(li.encoder->encode(word))));
#endif
        for (auto &s : result) {
            QByteArray ba(s.data(), int(s.size()));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            qtResult << li.codec->toUnicode(ba);
#else
            qtResult << QString(li.decoder->decode(ba));
#endif
        }
    }
    return std::move(qtResult);
}

QList<SpellEngineInterface::DictInfo> HunspellEngine::loadedDicts() const
{
    QList<DictInfo> ret;
    foreach (const LangItem &li, languages) {
        ret.append(li.info);
    }
    return ret;
}

QStringList HunspellEngine::diagnostics() const { return QtNote::diagnostics; }

int HunspellEngine::findLangItem(const QLocale &locale)
{
    for (int i = 0; i < languages.count(); i++) {
        if (
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            languages[i].info.country == locale.country()
#else
            languages[i].info.country == locale.territory()
#endif
            && languages[i].info.language == locale.language())
            return i;
    }
    return -1;
}

}
