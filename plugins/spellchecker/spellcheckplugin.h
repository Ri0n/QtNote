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

#ifndef SpellCheckPlugin_H
#define SpellCheckPlugin_H

#include <QObject>

#include "qtnoteplugininterface.h"
#include "trayinterface.h"
#include "deintegrationinterface.h"

namespace QtNote {

class SpellEngineInterface;

class SpellCheckPlugin : public QObject, public QtNotePluginInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
	Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.spellchecker")
#endif
	Q_INTERFACES(QtNote::QtNotePluginInterface)
public:
	explicit SpellCheckPlugin(QObject *parent = 0);

	virtual PluginMetadata metadata();
	bool init(Main *qtnote);

private slots:
	void noteWidgetCreated(QWidget *w);

private:	
	SpellEngineInterface *sei;
};

} // namespace QtNote

#endif // SpellCheckPlugin_H
