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
class DictionaryDownloader;

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
    enum DictFlag {
        DictNone           = 0,
        DictUserSelected   = 0x1,
        DictSystemSelected = 0x2,
        DictActivated      = 0x4,
        DictInstalled      = 0x8,
        DictWritable       = 0x10, // locally installed / deletable
        DictInstalling     = 0x20, // installation in progress
    };
    Q_DECLARE_FLAGS(DictFlags, DictFlag)

    struct Dict {
        QLocale   locale;
        DictFlags flags;
    };

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

    QList<QLocale> userLanguagePreferences() const;
    QList<QLocale> systemLanguagePreferences() const;
    QList<Dict>    dictionaries() const;

    void                  removeDictionary(const QLocale &locale);
    DictionaryDownloader *download(const QLocale &locale);

signals:
    void availableDictsUpdated();
private slots:
    void noteWidgetCreated(QWidget *w);
    void settingsAccepted();

private:
    PluginHostInterface  *host;
    SpellEngineInterface *sei;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SpellCheckPlugin::DictFlags)

} // namespace QtNote

#endif // SpellCheckPlugin_H
