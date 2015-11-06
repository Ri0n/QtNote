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

#ifndef NOTEMANAGER_H
#define NOTEMANAGER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QLinkedList>
#include <QPointer>

#include "notestorage.h"
#include "qtnote_export.h"

namespace QtNote {

class QTNOTE_EXPORT GlobalNoteFinder : public QObject
{
    Q_OBJECT

    QHash<NoteFinder*,QPointer<NoteFinder> >  searchers;

public:
    GlobalNoteFinder(QObject *parent = 0);

    void start(const QString &text);
    void abort();

signals:
    void found(const QString &storageId, const QString &noteId);
    void completed();

private slots:
    void noteFound(const QString &noteId);
    void searcherFinished();
};

class QTNOTE_EXPORT NoteManager : public QObject
{
    Q_OBJECT
public:
    static NoteManager *instance();
    static GlobalNoteFinder *search() { return new GlobalNoteFinder(instance()); }

    void registerStorage(NoteStorage::Ptr storage);
    void unregisterStorage(NoteStorage::Ptr storage);
    bool loadAll();

    QList<NoteListItem> noteList(int count = -1) const;

    Note note(const QString &storageId, const QString &noteId);

    const QMap<QString, NoteStorage::Ptr> storages(bool withInvalid = false) const;
    const QLinkedList<NoteStorage::Ptr> prioritizedStorages(bool withInvalid = false) const;

    NoteStorage::Ptr storage(const QString &storageId) const;
    inline NoteStorage::Ptr defaultStorage() const
    { return prioritizedStorages().isEmpty()?
                    NoteStorage::Ptr() : prioritizedStorages().first(); }

    /*
     * Accepts storages identifiers. the first one is in higher priority.
     * Pass empty list for defaults
     */
    void setPriorities(const QStringList &storageCodes);

signals:
    void storageAdded(NoteStorage::Ptr);
    void storageAboutToBeRemoved(NoteStorage::Ptr);
    void storageRemoved(NoteStorage::Ptr);
    void storageChanged(NoteStorage::Ptr);

private slots:
    void storageChanged();

private:
    NoteManager(QObject *parent);

    static NoteManager *_instance;
    QStringList _priorities;
    QMap<QString, NoteStorage::Ptr> _storages;
    mutable QLinkedList<NoteStorage::Ptr> _prioCache;
};

}

#endif // NOTEMANAGER_H
