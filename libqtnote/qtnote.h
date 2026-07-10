/*
QtNote - Simple note-taking application
Copyright (C) 2010 Sergei Ilinykh

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
#include "qtnote_export.h"

class QAction;
class QMenu;

namespace QtNote {

class PluginManager;
class ShortcutsManager;
class NoteWidget;
class TrayImpl;
class DEIntegrationInterface;
class GlobalShortcutsInterface;
class NotificationInterface;
class NoteDialog;
class QtNoteDBus;
class Note;

class QTNOTE_EXPORT Main : public QObject {
    Q_OBJECT
public:
    explicit Main(QObject *parent = 0);
    ~Main();
    bool isOperable() const { return _inited; }
    void parseAppArguments(const QStringList &args);

    NoteWidget       *noteWidget(const Note &note);
    virtual void      activateWidget(QWidget *w) const; // virtual for plugins
    ShortcutsManager *shortcutsManager() const { return _shortcutsManager; }
    PluginManager    *pluginManager() const { return _pluginManager; }

    void setTrayImpl(TrayImpl *tray);
    void setExternalTrayAvailable(bool available);
    void setDesktopImpl(DEIntegrationInterface *de);
    void setGlobalShortcutsImpl(GlobalShortcutsInterface *gs);
    void setNotificationImpl(NotificationInterface *notifier);

    void registerStorage(NoteStorage::Ptr &storage);
    void unregisterStorage(NoteStorage::Ptr &storage);

private:
    NoteDialog *makeNoteDialog(const QString &storageId, const QString &noteId = {});

signals:
    void noteWidgetCreated(QWidget *);
    void settingsUpdated();

public slots:
    void notifyError(const QString &);
    void openNoteDialog(const QString &storageId, const QString &noteId);
    void appMessageReceived(const QString &message);

private slots:
    void exitQtNote();
    void showAbout();
    void showNoteManager();
    void showOptions();
    void createNewNote();
    void createNewNoteFromSelection();
    void note_trashRequested();
    void note_removed(const Note &noteItem);

private:
    class Private;
    Private *d;

    bool              _inited;
    ShortcutsManager *_shortcutsManager;
    PluginManager    *_pluginManager;
};

} // namespace QtNote

#endif // QTNOTE_H
