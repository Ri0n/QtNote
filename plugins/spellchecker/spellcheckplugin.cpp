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
#include "noteedit.h"
#include "notehighlighter.h"
#include "notewidget.h"
#include "qtnote.h"
#include "qtnote_config.h"
#include "settingsdlg.h"
#include "spellcheckplugin.h"

namespace QtNote {

static const QLatin1String pluginId("spellchecker");

static std::shared_ptr<HighlighterExtension> hlExt;

class SpellCheckHighlighterExtension : public HighlighterExtension {
    SpellEngineInterface *sei;
    QRegularExpression    expression;

public:
    SpellCheckHighlighterExtension(SpellEngineInterface *sei) : sei(sei)
    {
        expression = QRegularExpression("[[:alpha:]]{2,}", QRegularExpression::UseUnicodePropertiesOption);
    }

    void reset() { }

    void highlight(NoteHighlighter *nh, const QString &text)
    {
        QTextCharFormat myClassFormat;
        myClassFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

        QRegularExpressionMatchIterator i = expression.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            // qDebug() << match.captured();
            if (!sei->spell(match.captured())) {
                nh->addFormat(match.capturedStart(), match.capturedLength(), myClassFormat);
            }
        }
    }
};

class SpellContextMenu : public QObject {
    Q_OBJECT

    SpellEngineInterface *sei;
    QTextEdit            *te;
    QTextCursor           cursor;

public:
    SpellContextMenu(SpellEngineInterface *sei, QTextEdit *te, const QTextCursor &cursor, QString wrongWord,
                     QMenu *menu) : QObject(menu), sei(sei), te(te), cursor(cursor)
    {
        QList<QString>   suggestions = sei->suggestions(wrongWord);
        QList<QAction *> actions;
        foreach (QString suggestion, suggestions) {
            QAction *act_suggestion = new QAction(suggestion, menu);
            actions.append(act_suggestion);
            connect(act_suggestion, SIGNAL(triggered()), SLOT(applySuggestion()));
        }
        if (actions.count()) {
            QAction *sep = new QAction(menu);
            sep->setSeparator(true);
            actions.append(sep);
        }
        QAction *act_add = new QAction(tr("Add to dictionary"), menu);
        act_add->setData(wrongWord);
        actions.append(act_add);
        connect(act_add, SIGNAL(triggered()), SLOT(addToDictionary()));
        QAction *sep = new QAction(menu);
        sep->setSeparator(true);
        actions.append(sep);
        menu->insertActions(menu->actions().value(0), actions);
    }

signals:
    void needRehighlight();

private slots:
    void applySuggestion()
    {
        QString word      = ((QAction *)sender())->text();
        int     oldPos    = te->textCursor().position();
        int     oldLength = cursor.position() - cursor.anchor();

        cursor.insertText(word);
        cursor.clearSelection();

        // Put the cursor where it belongs
        cursor.setPosition(oldPos - oldLength + word.length());
        te->setTextCursor(cursor);
    }

    void addToDictionary()
    {
        QString word = ((QAction *)sender())->data().toString();
        sei->addToDictionary(word);
        emit needRehighlight();
    }
};

//------------------------------------------------------------
// SpellCheckPlugin
//------------------------------------------------------------
SpellCheckPlugin::SpellCheckPlugin(QObject *parent) : QObject(parent), sei(0) { }

SpellCheckPlugin::~SpellCheckPlugin() { delete sei; }

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
    sei        = new HunspellEngine(host);
    auto langs = userLanguagePreferences();
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
    hlExt = std::make_shared<SpellCheckHighlighterExtension>(sei);
    connect(qtnote, &Main::noteWidgetCreated, this, &SpellCheckPlugin::noteWidgetCreated);
    connect(sei, &HunspellEngine::availableDictsUpdated, this, &SpellCheckPlugin::availableDictsUpdated);
    return true;
}

QString SpellCheckPlugin::tooltip() const
{
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

void SpellCheckPlugin::populateNoteContextMenu(QTextEdit *te, QContextMenuEvent *event, QMenu *menu)
{
    auto last_click_ = event->pos();
    if (!te->textCursor().selection().isEmpty()) {
        return;
    }
    // Check if the word under the cursor is misspelled
    QTextCursor cursor = te->cursorForPosition(last_click_);
    cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    QString wrongWord = cursor.selectedText();
    if (!wrongWord.isEmpty() && !sei->spell(wrongWord)) {
        auto cm = new SpellContextMenu(sei, te, cursor, wrongWord, menu);
        connect(cm, &SpellContextMenu::needRehighlight, this, [this]() { host->rehighlight(); });
    }
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

void SpellCheckPlugin::removeDictionary(const QLocale &locale) { sei->removeDictionary(locale); }

QList<SpellCheckPlugin::Dict> SpellCheckPlugin::dictionaries() const
{
    auto systemPrefs = systemLanguagePreferences();
    auto userPrefs   = userLanguagePreferences();
    auto downloaded  = sei->supportedLanguages();

    QList<SpellCheckPlugin::Dict> ret;
    for (auto const &d : std::as_const(downloaded)) {
        DictFlags flags = DictInstalled;
        flags |= (d.isWritable ? DictWritable : DictNone);
        flags |= (d.isLoaded ? DictActivated : DictNone);
        ret.append({QLocale(d.language, d.country), flags});
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
    auto      dlg = (SettingsDlg *)sender();
    auto      ret = dlg->preferredList();
    QSettings s;
    s.beginGroup("plugins");
    s.beginGroup(pluginId);
    const auto prevLangs = s.value(QLatin1String("langs")).toStringList();

    QStringList langs;
    foreach (const QLocale &locale, ret) {
        langs.append(locale.bcp47Name());
        sei->addLanguage(locale);
    }
    for (const auto &p : prevLangs) {
        if (!langs.contains(p)) {
            sei->removeLanguage(QLocale(p));
        }
    }
    s.setValue(QLatin1String("langs"), langs);
}

void SpellCheckPlugin::noteWidgetCreated(QWidget *w)
{
    host->addHighlightExtension(w, hlExt, int(NoteHighlighter::SpellCheck));
    host->noteTextWidget(w)->addContextMenuHandler(this);
}

} // namespace QtNote

#include "spellcheckplugin.moc"
