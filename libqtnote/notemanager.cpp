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

#include <QApplication>
#include <QStringList>
#include <QLinkedList>
#include <QSettings>

#include "notemanager.h"

namespace QtNote {

NoteManager::NoteManager(QObject *parent)
    : QObject(parent)
{
    _priorities = QSettings().value("storage.priority").toStringList();
}

void NoteManager::registerStorage(NoteStorage::Ptr storage)
{
    _prioCache.clear();
    _storages.insert(storage->systemName(), storage);
    if (!_priorities.contains(storage->systemName())) {
        if (storage->systemName() == QLatin1String("ptf")) {
            _priorities.prepend(storage->systemName()); // highest priority for ptf if not set
        } else {
            _priorities.append(storage->systemName()); // lowest priority if not set
        }
    }

    connect(storage.data(), SIGNAL(invalidated()), SLOT(storageChanged()));
    connect(storage.data(), SIGNAL(noteAdded(NoteListItem)), SLOT(storageChanged()));
    connect(storage.data(), SIGNAL(noteModified(NoteListItem)), SLOT(storageChanged()));
    connect(storage.data(), SIGNAL(noteRemoved(NoteListItem)), SLOT(storageChanged()));

    emit storageAdded(storage);
}

void NoteManager::unregisterStorage(NoteStorage::Ptr storage)
{
    emit storageAboutToBeRemoved(storage);
    _prioCache.clear();
    disconnect(storage.data());
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

const QLinkedList<NoteStorage::Ptr> NoteManager::prioritizedStorages() const
{
    if (_prioCache.size()) {
        return _prioCache;
    }

    auto storages = _storages;

    for (auto code: _priorities) {
        auto storage = storages.take(code);
        if (storage) {
            _prioCache.append(storage);
        }
    }

    for (auto storage: storages) {
        _prioCache.append(storage);
    }

    return _prioCache;
}

NoteStorage::Ptr NoteManager::storage(const QString &storageId) const
{
    // returns 0 if doesn't exists (see default contstructor StorageItem)
    return _storages.value(storageId);
}

void NoteManager::setPriorities(const QStringList &storageCodes)
{
    _prioCache.clear();
    _priorities = storageCodes;
    QSettings().setValue("storage.priority", _priorities);
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
