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

#include "notemanager.h"
#include <QApplication>
#include <QStringList>

namespace QtNote {

NoteManager::NoteManager(QObject *parent)
    : QObject(parent)
{

}

void NoteManager::registerStorage(NoteStorage::Ptr storage, quint16 priority)
{
    _storages.insert(storage->systemName(), storage);
    while (_prioritizedStorages.contains(priority)) {
        priority++;
    }
    _prioritizedStorages.insert(priority, storage);

    connect(storage.data(), SIGNAL(invalidated()), SLOT(storageChanged()));
    connect(storage.data(), SIGNAL(noteAdded(NoteListItem)), SLOT(storageChanged()));
    connect(storage.data(), SIGNAL(noteModified(NoteListItem)), SLOT(storageChanged()));
    connect(storage.data(), SIGNAL(noteRemoved(NoteListItem)), SLOT(storageChanged()));

    emit storageAdded(storage);
}

void NoteManager::unregisterStorage(NoteStorage::Ptr storage)
{
    emit storageAboutToBeRemoved(storage);
    disconnect(storage.data());
    _prioritizedStorages.remove(_prioritizedStorages.key(storage));
    _storages.remove(storage->systemName());
    emit storageRemoved(storage);
}

bool NoteManager::loadAll()
{
    // mostly stubbed method for future use.
    if (_storages.count() == 0) {
        return false;
    }
    return true;
}

void NoteManager::storageChanged()
{
    emit storageChanged(_storages[((NoteStorage*)sender())->systemName()]);
}

QList<NoteListItem> NoteManager::noteList(int count) const
{
    // TODO optimize
    QList<NoteListItem> ret;
    foreach (NoteStorage::Ptr storage, prioritizedStorages()) {
        QList<NoteListItem> items = storage->noteList();
        qSort(items.begin(), items.end(), noteListItemModifyComparer);
        ret += items;
    }
    return ret.mid(0, count);
}

Note NoteManager::getNote(const QString &storageId, const QString &noteId)
{
    NoteStorage::Ptr s = storage(storageId);
    if (s) {
        return s->get(noteId);
    }
    return 0;
}

const QMap<QString, NoteStorage::Ptr> NoteManager::storages() const
{
    return _storages;
}

NoteStorage::Ptr NoteManager::storage(const QString &storageId) const
{
    // returns 0 if doesn't exists (see default contstructor StorageItem)
    return _storages.value(storageId);
}

void NoteManager::updatePriorities(const QStringList &storageCodes)
{
    QMap<quint16, NoteStorage::Ptr> p;

    for (quint16 i=0; i<storageCodes.count(); i++) {
        p[i] = _storages[storageCodes[i]];
    }
    _prioritizedStorages = p;
}

/*int NoteManager::notesAmount(const QString &storage = QString()) const
{
    int result = 0;
    if (storage.isEmpty()) {
        foreach (const StorageItem &s, storages()) {
            result += s.storage->notesAmount();
        }
    } else if (storages().contains(storage)) {
        result += storages().value(storage).storage->notesAmount();
    }
    return result;
}*/

NoteManager *NoteManager::instance()
{
    if (!NoteManager::_instance) {
        NoteManager::_instance = new NoteManager(QApplication::instance());
    }
    return NoteManager::_instance;
}

NoteManager* NoteManager::_instance = NULL;

} // namespace QtNote
