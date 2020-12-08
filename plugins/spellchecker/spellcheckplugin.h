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

#include "deintegrationinterface.h"
#include "notecontextmenuhandler.h"
#include "pluginoptionsinterface.h"
#include "qtnoteplugininterface.h"
#include "trayinterface.h"

namespace QtNote {

class SpellEngineInterface;
class PluginHostInterface;

class SpellCheckPlugin : public QObject,
                         public PluginInterface,
                         public RegularPluginInterface,
                         public PluginOptionsTooltipInterface,
                         public PluginOptionsInterface,
                         public NoteContextMenuHandler {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.rion-soft.QtNote.spellchecker")
    Q_INTERFACES(QtNote::PluginInterface QtNote::RegularPluginInterface QtNote::PluginOptionsTooltipInterface
                     QtNote::PluginOptionsInterface QtNote::NoteContextMenuHandler)
public:
    explicit SpellCheckPlugin(QObject *parent = 0);
    ~SpellCheckPlugin();

    // PluginInterface
    int                    metadataVersion() const;
    virtual PluginMetadata metadata();
    void                   setHost(PluginHostInterface *host);

    // RegularPluginInterface
    bool init(Main *qtnote);

    // PluginOptionsTooltipInterface
    QString tooltip() const;

    // PluginOptionsInterface
    QDialog *optionsDialog();

    // NoteContextMenuHandler
    void populateNoteContextMenu(QTextEdit *te, QContextMenuEvent *event, QMenu *menu);

    inline SpellEngineInterface *engine() const { return sei; }
    QList<QLocale>               preferredLanguages() const;

private slots:
    void noteWidgetCreated(QWidget *w);
    void settingsAccepted();

private:
    PluginHostInterface * host;
    SpellEngineInterface *sei;
};

} // namespace QtNote

#endif // SpellCheckPlugin_H
