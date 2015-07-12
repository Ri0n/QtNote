/*
QtNote - Simple note-taking application
Copyright (C) 2010 Ili'nykh Sergey

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

#ifndef QTNOTE_H
#define QTNOTE_H

#include <QObject>
#include "notestorage.h"

class QAction;
class QMenu;

namespace QtNote {

class PluginManager;
class ShortcutsManager;
class NoteWidget;
class TrayImpl;
class DEIntegrationInterface;
class GlobalShortcutsInterface;
class NoteListItem;

class Main : public QObject
{
    Q_OBJECT
public:
    explicit Main(QObject *parent = 0);
    ~Main();
    inline bool isOperable() const { return _inited; }
    void parseAppArguments(const QStringList &args);

    NoteWidget *noteWidget(const QString &storageId, const QString &noteId);
    void notifyError(const QString &);
    void activateWidget(QWidget *w) const;
    inline ShortcutsManager* shortcutsManager() const { return _shortcutsManager; }
    inline PluginManager* pluginManager() const { return _pluginManager; }

    void setTrayImpl(TrayImpl *tray);
    void setDesktopImpl(DEIntegrationInterface *de);
    void setGlobalShortcutsImpl(GlobalShortcutsInterface *gs);
    bool registerStorage(NoteStorage::Ptr &storage);
    void unregisterStorage(NoteStorage::Ptr &storage);

signals:
    void noteWidgetCreated(QWidget*);
    void noteWidgetInitiated(QWidget*);
    void settingsUpdated();

public slots:
    void showNoteDialog(const QString &storageId, const QString &noteId = QString::null, const QString &contents = QString::null);

private slots:
    void exitQtNote();
    void showAbout();
    void showNoteManager();
    void showOptions();
    void createNewNote();
    void createNewNoteFromSelection();
    void appMessageReceived(const QString &msg);
    void note_trashRequested();
    void note_saveRequested();
    void note_invalidated();
    void note_removed(const NoteListItem &noteItem);

private:
    class Private;
    Private *d;

    bool _inited;
    ShortcutsManager* _shortcutsManager;
    PluginManager *_pluginManager;

};

} // namespace QtNote

#endif // QTNOTE_H
