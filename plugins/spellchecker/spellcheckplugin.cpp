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

#include "spellcheckplugin.h"
#include "qtnote.h"
#include "notewidget.h"
#include "hunspellengine.h"
#include "highlighterext.h"
#include "notehighlighter.h"
#include "settingsdlg.h"

namespace QtNote {

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

PluginMetadata SpellCheckPlugin::metadata()
{
	PluginMetadata md;
	md.icon = QIcon(":/icons/spellcheck-logo");
	md.name = "Spell check";
	md.description = "Realtime spell check.";
	md.author = "Sergey Il'inykh <rion4ik@gmail.com>";
	md.version = 0x010000;	// plugin's version 0xXXYYZZPP
	md.minVersion = 0x020300; // minimum compatible version of QtNote
	md.maxVersion = 0x030000; // maximum compatible version of QtNote
	md.homepage = QUrl("http://ri0n.github.io/QtNote");
	return md;
}

bool SpellCheckPlugin::init(Main *qtnote)
{
	sei = new HunspellEngine();
	QLocale systemLocale = QLocale::system();
	QLocale enLocale = QLocale(QLocale::English, QLocale::UnitedStates);
	sei->addLanguage(QLocale::system());
    if (enLocale.language() != systemLocale.language() || enLocale.country() != systemLocale.country()) {
		sei->addLanguage(enLocale);
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
	auto s = new SettingsDlg;
	connect(s, SIGNAL(accepted()), SLOT(settingsAccepted()));
	return s;
}

void SpellCheckPlugin::settingsAccepted()
{
	auto dlg = (SettingsDlg*)sender();
	auto ret = dlg->preferredList();

}

void SpellCheckPlugin::noteWidgetCreated(QWidget *w)
{
	NoteWidget *nw = dynamic_cast<NoteWidget*>(w);
	nw->highlighter()->addExtension(hlExt);
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(spellcheckplugin, QtNote::SpellCheckPlugin)
#endif
