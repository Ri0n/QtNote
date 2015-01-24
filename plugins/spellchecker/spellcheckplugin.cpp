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
#include <QProcess>
#include <QTimer>

#include "spellcheckplugin.h"
#include "qtnote.h"
#include "notewidget.h"
#include "spellchecker.h"

namespace QtNote {


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
	md.pluginType = PluginMetadata::DEIntegration;
	md.icon = QIcon(":/icons/ubuntu-logo");
	md.name = "Spell check";
	md.description = "Realtime spell check.";
	md.author = "Sergey Il'inykh <rion4ik@gmail.com>";
	md.version = 0x010000;	// plugin's version 0xXXYYZZPP
	md.minVersion = 0x020300; // minimum compatible version of QtNote
	md.maxVersion = 0x030000; // maximum compatible version of QtNote
	return md;
}

bool SpellCheckPlugin::init(Main *qtnote)
{
	connect(qtnote, SIGNAL(noteWidgetCreated(QWidget*)), SLOT(noteWidgetCreated(QWidget*)));
	return true;
}

void SpellCheckPlugin::noteWidgetCreated(QWidget *w)
{
	NoteWidget *nw = dynamic_cast<NoteWidget*>(w);

	new SpellChecker(nw->editWidget());
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(spellcheckplugin, QtNote::SpellCheckPlugin)
#endif
