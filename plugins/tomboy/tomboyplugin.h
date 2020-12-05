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

#ifndef TOMBOYPLUGIN_H
#define TOMBOYPLUGIN_H

#include <QObject>

#include "qtnoteplugininterface.h"

namespace QtNote {

class TomboyPlugin : public QObject, public PluginInterface, public RegularPluginInterface {
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.tomboy")
#endif
    Q_INTERFACES(QtNote::PluginInterface QtNote::RegularPluginInterface)

    Main *qtnote;

public:
    explicit TomboyPlugin(QObject *parent = 0);
    ~TomboyPlugin();

    int                    metadataVersion() const;
    virtual PluginMetadata metadata();
    bool                   init(Main *qtnote);
};

} // namespace QtNote

#endif // TOMBOYPLUGIN_H
