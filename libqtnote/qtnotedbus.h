/*
    SPDX-License-Identifier: GPL-3.0-only
*/

#ifndef QTNOTE_DBUS_H
#define QTNOTE_DBUS_H

#include "qtnote_export.h"

#include <QObject>
#include <QString>

namespace QtNote {

class Main;

class QTNOTE_EXPORT QtNoteDBus : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.github.ri0n.QtNote")

public:
    explicit QtNoteDBus(Main *qtnote, QObject *parent = nullptr);
    ~QtNoteDBus() override;

signals:
    Q_SCRIPTABLE void notesChanged();
    Q_SCRIPTABLE void globalShortcutsChanged();

    void createNoteRequested();
    void globalShortcutActivated(const QString &id);
    void noteManagerRequested();
    void optionsRequested();
    void aboutRequested();
    void quitRequested();
    void openNoteRequested(const QString &storageId, const QString &noteId);

public slots:
    Q_SCRIPTABLE QString notesJson(int offset, int limit, const QString &query) const;
    Q_SCRIPTABLE QString globalShortcutsJson() const;
    Q_SCRIPTABLE void    setXdgActivationToken(const QString &token);
    Q_SCRIPTABLE void    openNote(const QString &storageId, const QString &noteId);
    Q_SCRIPTABLE void    createNote();
    Q_SCRIPTABLE void    activateGlobalShortcut(const QString &id);
    Q_SCRIPTABLE void    showNoteManager();
    Q_SCRIPTABLE void    showOptions();
    Q_SCRIPTABLE void    showAbout();
    Q_SCRIPTABLE void    quit();

private:
    Main *m_qtnote;
    bool  m_registeredService = false;
};

} // namespace QtNote

#endif // QTNOTE_DBUS_H
