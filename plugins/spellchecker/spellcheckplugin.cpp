/*
QtNote - Simple note-taking application
Copyright (C) 2015 Sergei Ilinykh

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Contacts:
E-Mail: rion4ik@gmail.com XMPP: rion@jabber.ru
*/

#include <QContextMenuEvent>
#include <QDebug>
#include <QDialog>
#include <QHBoxLayout>
#include <QLocale>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QTextDocumentFragment>
#include <QWidget>
#include <QtPlugin>
#include <memory>

#include "dictionarydownloader.h"
#include "highlighterext.h"
#include "hunspellengine.h"
#include "notewidget.h"
#include "qtnote.h"
#include "qtnote_config.h"
#include "settingsdlg.h"
#include "spellcheckplugin.h"
#include "spellcheckprovider.h"

namespace QtNote {

static const QLatin1String pluginId("spellchecker");

class HunspellProvider final : public SpellCheckProvider {
public:
    explicit HunspellProvider(std::shared_ptr<SpellEngineInterface> engine) : engine_(std::move(engine)) { }

    QString     id() const override { return QStringLiteral("hunspell"); }
    QString     displayName() const override { return QStringLiteral("Hunspell"); }
    bool        isValid() const override { return engine_ && !engine_->loadedDicts().isEmpty(); }
    bool        isCorrect(const QString &word) const override { return enabled_ && engine_->spell(word); }
    QStringList suggestions(const QString &word) const override { return engine_->suggestions(word); }
    void        addToDictionary(const QString &word) override { engine_->addToDictionary(word); }

protected:
    void onDisabled(DisableMode) override { enabled_ = false; }

private:
    std::shared_ptr<SpellEngineInterface> engine_;
    bool                                  enabled_ = true;
};

//------------------------------------------------------------
// SpellCheckPlugin
//------------------------------------------------------------
SpellCheckPlugin::SpellCheckPlugin(QObject *parent) : QObject(parent), qtnote_(0), host(0), sei(0) { }

SpellCheckPlugin::~SpellCheckPlugin() { deinit(); }

int SpellCheckPlugin::metadataVersion() const { return MetadataVersion; }

PluginMetadata SpellCheckPlugin::metadata()
{
    PluginMetadata md;
    md.id          = pluginId;
    md.icon        = QIcon(":/icons/spellcheck-logo");
    md.name        = "Spell check";
    md.description = tr("Realtime spell check.");
    md.author      = "Sergei Ilinykh <rion4ik@gmail.com>";
    md.version     = 0x020000;       // plugin's version 0xXXYYZZPP
    md.minVersion  = 0x020300;       // minimum compatible version of QtNote
    md.maxVersion  = QTNOTE_VERSION; // maximum compatible version of QtNote
    md.homepage    = QUrl("http://ri0n.github.io/QtNote");
    return md;
}

void SpellCheckPlugin::setHost(PluginHostInterface *host) { this->host = host; }

bool SpellCheckPlugin::init(Main *qtnote)
{
    deinit();
    initializationFailure_.clear();
    qtnote_      = qtnote;
    engineOwner_ = std::make_shared<HunspellEngine>(host);
    sei          = engineOwner_.get();
    auto langs   = userLanguagePreferences();
    if (langs.empty()) {
        langs = systemLanguagePreferences();
    }
    QSettings s;
    s.beginGroup(QLatin1String("plugins"));
    s.beginGroup(pluginId);
    auto failedDownloads = s.value("failed-dl").toStringList();

    foreach (auto &l, langs) {
        if (sei->addLanguage(l) == SpellEngineInterface::NotInstalledError && sei->canDownload(l)
            && !failedDownloads.contains(l.name())) {
            auto downloader = download(l);
            connect(downloader, &DictionaryDownloader::finished, this, [this, l, downloader]() {
                if (!downloader->hasErrors()) {
                    sei->addLanguage(l);
                }
            });
        }
    }
    qInfo() << "Spell checker initialized with" << sei->loadedDicts().size() << "dictionary/dictionaries";
    auto       provider = std::make_shared<HunspellProvider>(engineOwner_);
    const auto accepted = host->offerSpellCheckProvider(provider);
    qInfo() << "Hunspell provider" << (accepted ? "accepted" : "not selected");
    if (!accepted) {
        const auto failure = provider->disabledReason();
        deinit();
        initializationFailure_ = failure;
        qInfo() << "Hunspell dictionaries unloaded after provider rejection";
        return false;
    }
    connect(sei, &HunspellEngine::availableDictsUpdated, this, &SpellCheckPlugin::availableDictsUpdated);
    return true;
}

void SpellCheckPlugin::deinit()
{
    if (qtnote_) {
        qtnote_ = nullptr;
    }
    if (sei) {
        disconnect(sei, nullptr, this, nullptr);
        sei = nullptr;
    }
    engineOwner_.reset();
    if (host) {
        host->rehighlight();
    }
}

QString SpellCheckPlugin::tooltip() const
{
    if (!sei) {
        return initializationFailure_.isEmpty() ? tr("Spell checker is disabled.") : initializationFailure_;
    }
    QList<SpellEngineInterface::DictInfo> dicts = sei->loadedDicts();
    QStringList                           dictsHtml;
    foreach (auto &d, dicts) {
        QLocale locale(d.language, d.country);
        QString l = QLatin1String("<i>") + locale.nativeLanguageName() + QLatin1String(" (")
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            + locale.nativeTerritoryName() + QLatin1Char(')');
#else
            + locale.nativeCountryName() + QLatin1Char(')');
#endif
        if (!d.filename.isEmpty()) {
            l += QLatin1String(":</i> ");
            l += d.filename;
        } else {
            l += QLatin1String("</i>");
        }
        dictsHtml.append(l);
    }

    auto ret = tr("<b>Loaded dictionaries:</b> ") + (dictsHtml.isEmpty() ? tr("none") : dictsHtml.join("<br/>  "));
    if (dicts.empty() && sei->supportedLanguages().isEmpty()) {
        ret += QString("<br/><b>%1:</b><br/>").arg(tr("Diagnostics"));
        ret += sei->diagnostics().join(QLatin1String("<br/>"));
    }
    return ret;
}

QDialog *SpellCheckPlugin::optionsDialog()
{
    auto s = new SettingsDlg(this);
    connect(s, SIGNAL(accepted()), SLOT(settingsAccepted()));
    return s;
}

QList<QLocale> SpellCheckPlugin::systemLanguagePreferences() const
{
    QLocale     systemLocale = QLocale::system();
    QStringList uiLangs      = systemLocale.uiLanguages();

    QList<QLocale> ret;
    ret.reserve(uiLangs.size() + 1);
    foreach (const QString &uiLang, uiLangs) {
        auto locale = QLocale(uiLang);
        if (!ret.contains(locale)) {
            ret.append(locale);
        }
    }

    QLocale enLocale = QLocale(QLocale::English, QLocale::UnitedStates);
    if (!ret.contains(enLocale)) {
        ret.append(enLocale);
    }
    return ret;
}

void SpellCheckPlugin::removeDictionary(const QLocale &locale)
{
    if (sei) {
        sei->removeDictionary(locale);
    }
}

QList<SpellCheckPlugin::Dict> SpellCheckPlugin::dictionaries() const
{
    if (!sei) {
        return {};
    }
    auto systemPrefs = systemLanguagePreferences();
    auto userPrefs   = userLanguagePreferences();
    auto downloaded  = sei->supportedLanguages();

    QList<SpellCheckPlugin::Dict> ret;
    for (auto const &d : std::as_const(downloaded)) {
        DictFlags flags = DictInstalled;
        flags |= (d.isWritable ? DictWritable : DictNone);
        flags |= (d.isLoaded ? DictActivated : DictNone);
        ret.append({ QLocale(d.language, d.country), flags });
    }

    QList<SpellCheckPlugin::Dict> systemPrefDicts;
    for (auto const &locale : std::as_const(systemPrefs)) {
        auto it = std::ranges::find_if(ret, [&locale](auto const &d) { return d.locale == locale; });
        if (it != ret.end()) {
            it->flags |= DictSystemSelected;
            systemPrefDicts.push_back(*it);
            ret.erase(it);
        } else {
            if (sei->canDownload(locale)) {
                DictFlags flags = (DictSystemSelected | DictWritable);
                systemPrefDicts.push_back({ locale, flags });
            }
        }
    }
    ret = systemPrefDicts + ret;

    for (auto &d : ret) {
        d.flags |= (userPrefs.contains(d.locale) ? DictUserSelected : DictNone);
    }
    return ret;
}

DictionaryDownloader *SpellCheckPlugin::download(const QLocale &locale)
{
    if (!sei) {
        return nullptr;
    }
    auto downloader = sei->download(locale);
    connect(downloader, &DictionaryDownloader::finished, this, [locale, downloader]() {
        QSettings s;
        s.beginGroup(QLatin1String("plugins"));
        s.beginGroup(pluginId);
        auto failedDownloads = s.value("failed-dl").toStringList();
        auto name            = locale.name();
        auto wasFailed       = failedDownloads.contains(name);

        if (downloader->hasErrors()) {
            if (!wasFailed) {
                failedDownloads.append(name);
                s.setValue("failed-dl", failedDownloads);
            }
        } else {
            if (wasFailed) {
                failedDownloads.removeOne(name);
                if (failedDownloads.isEmpty()) {
                    s.remove("failed-dl");
                } else {
                    s.setValue("failed-dl", failedDownloads);
                }
            }
        }
    });
    return downloader;
}

QList<QLocale> SpellCheckPlugin::userLanguagePreferences() const
{
    QSettings s;
    s.beginGroup(QLatin1String("plugins"));
    s.beginGroup(pluginId);

    auto           userPrefs = s.value(QLatin1String("langs")).toStringList();
    QList<QLocale> ret;
    ret.reserve(userPrefs.size());
    foreach (const QString &code, userPrefs) {
        QLocale locale(code);
        if (locale != QLocale::c()) {
            ret.append(locale);
        }
    }
    return ret;
}

void SpellCheckPlugin::settingsAccepted()
{
    if (!sei) {
        return;
    }
    auto       dlg             = (SettingsDlg *)sender();
    auto       ret             = dlg->preferredList();
    const auto activeLanguages = ret.isEmpty() ? systemLanguagePreferences() : ret;
    QSettings  s;
    s.beginGroup("plugins");
    s.beginGroup(pluginId);
    QStringList langs;
    foreach (const QLocale &locale, activeLanguages) {
        sei->addLanguage(locale);
    }
    foreach (const QLocale &locale, ret) {
        langs.append(locale.bcp47Name());
    }
    for (const auto &dict : sei->loadedDicts()) {
        const auto locale = dict.toLocale();
        if (!activeLanguages.contains(locale))
            sei->removeLanguage(locale);
    }
    s.setValue(QLatin1String("langs"), langs);
}

} // namespace QtNote
