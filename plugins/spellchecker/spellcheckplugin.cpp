/*
QtNote - Simple note-taking application
Copyright (C) 2015 Ili'nykh Sergey

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

#include <QWidget>
#include <QtPlugin>
#include <QLocale>
#include <QDialog>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QDebug>

#include "spellcheckplugin.h"
#include "qtnote.h"
#include "notewidget.h"
#include "hunspellengine.h"
#include "highlighterext.h"
#include "notehighlighter.h"
#include "settingsdlg.h"

namespace QtNote {

static const QLatin1String pluginId("spellchecker");

static HighlighterExtension::Ptr hlExt;

class SpellCheckHighlighterExtension : public HighlighterExtension
{
    SpellEngineInterface *sei;

public:
    SpellCheckHighlighterExtension(SpellEngineInterface *sei) : sei(sei) {}

    void highlight(NoteHighlighter *nh, const QString &text)
    {
        QTextCharFormat myClassFormat;
        myClassFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
        QString pattern = "\\b\\w{2,}\\b";

        QRegExp expression(pattern);
        int index = text.indexOf(expression);
        while (index >= 0) {
            int length = expression.matchedLength();
            if (sei->spell(expression.capturedTexts()[0]) == 0) {
                nh->addFormat(index, length, myClassFormat);
            }
            index = text.indexOf(expression, index + length);
        }
    }
};

//------------------------------------------------------------
// SpellCheckPlugin
//------------------------------------------------------------
SpellCheckPlugin::SpellCheckPlugin(QObject *parent) :
    QObject(parent)
{
}

int SpellCheckPlugin::metadataVersion() const
{
    return MetadataVerion;
}

PluginMetadata SpellCheckPlugin::metadata()
{
    PluginMetadata md;
    md.id = pluginId;
    md.icon = QIcon(":/icons/spellcheck-logo");
    md.name = "Spell check";
    md.description = tr("Realtime spell check.");
    md.author = "Sergey Il'inykh <rion4ik@gmail.com>";
    md.version = 0x010000;	// plugin's version 0xXXYYZZPP
    md.minVersion = 0x020300; // minimum compatible version of QtNote
    md.maxVersion = QTNOTE_VERSION; // maximum compatible version of QtNote
    md.homepage = QUrl("http://ri0n.github.io/QtNote");
    return md;
}

bool SpellCheckPlugin::init(Main *qtnote)
{
    sei = new HunspellEngine();
    foreach (auto &l, preferredLanguages()) {
        sei->addLanguage(l);
    }
    hlExt = HighlighterExtension::Ptr(new SpellCheckHighlighterExtension(sei));
    connect(qtnote, SIGNAL(noteWidgetCreated(QWidget*)), SLOT(noteWidgetCreated(QWidget*)));
    return true;
}

QString SpellCheckPlugin::tooltip() const
{
    QList<SpellEngineInterface::DictInfo> dicts = sei->loadedDicts();
    QStringList ret;
    foreach (auto &d, dicts) {
        QLocale locale(d.language, d.country);
        QString l = QLatin1String("<i>") + locale.nativeLanguageName() + QLatin1String(" (") +
                locale.nativeCountryName() + QLatin1Char(')');
        if (!d.filename.isEmpty()) {
            l += QLatin1String(":</i> ");
            l += d.filename;
        } else {
            l += QLatin1String("</i>");
        }
        ret.append(l);
    }

    return tr("<b>Loaded dictionaries:</b> ") + ret.join("<br/>  ");
}

QDialog *SpellCheckPlugin::optionsDialog()
{
    auto s = new SettingsDlg(this);
    connect(s, SIGNAL(accepted()), SLOT(settingsAccepted()));
    return s;
}

QList<QLocale> SpellCheckPlugin::preferredLanguages() const
{
    QSettings s;
    s.beginGroup(QLatin1String("plugins"));
    s.beginGroup(pluginId);

    if (!s.contains(QLatin1String("langs"))) {
        QHash<QString, QLocale> codes;
        QLocale systemLocale = QLocale::system();
        codes.insert(systemLocale.bcp47Name(), systemLocale);

        QStringList uiLangs = systemLocale.uiLanguages();
        foreach (const QString &uiLang, uiLangs) {
            codes.insert(uiLang, QLocale(uiLang));
        }

        QLocale enLocale = QLocale(QLocale::English, QLocale::UnitedStates);
        codes.insert(enLocale.bcp47Name(), enLocale);
        s.setValue(QLatin1String("langs"), QStringList(codes.keys()));
    }
    QList<QLocale> ret;
    foreach (const QString &code, s.value(QLatin1String("langs")).toStringList()) {
        QLocale locale(code);
        if (locale != QLocale::c()) {
            ret.append(locale);
        }
    }
    return ret;
}

void SpellCheckPlugin::settingsAccepted()
{
    auto dlg = (SettingsDlg*)sender();
    auto ret = dlg->preferredList();
    QSettings s;
    s.beginGroup("plugins");
    s.beginGroup(pluginId);
    QStringList langs;
    foreach (const QLocale &locale, ret) {
        langs.append(locale.bcp47Name());
    }
    s.setValue(QLatin1String("langs"), langs);
}

void SpellCheckPlugin::noteWidgetCreated(QWidget *w)
{
    NoteWidget *nw = dynamic_cast<NoteWidget*>(w);
    nw->highlighter()->addExtension(hlExt, NoteHighlighter::SpellCheck);
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(spellcheckplugin, QtNote::SpellCheckPlugin)
#endif
