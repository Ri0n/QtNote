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

#include <QtPlugin>

#include "qtnote.h"
#include "tomboyplugin.h"
#include "tomboystorage.h"

namespace QtNote {


//------------------------------------------------------------
// TomboyPlugin
//------------------------------------------------------------
TomboyPlugin::TomboyPlugin(QObject *parent) :
	QObject(parent)
{
}

PluginMetadata TomboyPlugin::metadata()
{
	PluginMetadata md;
	md.icon = QIcon(":/icons/tomboy");
	md.name = "Tomboy Storage";
	md.description = "Allows read and write tomboy notes";
	md.author = "Sergey Il'inykh <rion4ik@gmail.com>";
	md.version = 0x010000;	// plugin's version 0xXXYYZZPP
	md.minVersion = 0x020300; // minimum compatible version of QtNote
	md.maxVersion = 0x030000; // maximum compatible version of QtNote
	return md;
}

bool TomboyPlugin::init(Main *qtnote)
{
	auto storage = new TomboyStorage(this);
	if (!qtnote->registerStorage(storage)) {
		delete storage;
		return false;
	}
	return true;
}

} // namespace QtNote

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(TomboyPlugin, QtNote::TomboyPlugin)
#endif
