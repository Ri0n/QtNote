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
#include <QSettings>
#include <QStringList>

#include <list>

namespace QtNote {

/**
 * @brief Notes search class.
 * Search object is owned by storage and self-destroyed on search-end
 */
class NoteFinder : public QObject {
    Q_OBJECT

protected:
    NoteStorage &_storage;

public:
    NoteFinder(NoteStorage &storage) : QObject(&storage), _storage(storage) { }

    inline NoteStorage &storage() const { return _storage; }

signals:
    void found(const QString &noteId);
    void completed();

public slots:
    virtual void start(const QString &text);
    virtual void abort();
};

void NoteFinder::start(const QString &text)
{
    auto nl = _storage.noteList();
    for (auto &n : qAsConst(nl)) {
        // text always returns plain text
        if (_storage.note(n.id).text().contains(text)) {
            emit found(n.id);
        }
    }

    emit completed();
    deleteLater();
}

void NoteFinder::abort() { }

GlobalNoteFinder::GlobalNoteFinder(QObject *parent) : QObject(parent) { }

void GlobalNoteFinder::start(const QString &text)
{
    abort();
    for (auto &storage : NoteManager::instance()->storages()) {
        auto so = new NoteFinder(*storage);
        connect(so, SIGNAL(found(QString)), SLOT(noteFound(QString)));
        connect(so, SIGNAL(completed()), SLOT(searcherFinished()));
        searchers.insert(so, so);
        so->start(text);
    }
}

void GlobalNoteFinder::abort()
{
    for (auto s : qAsConst(searchers)) {
        if (s) {
            s->abort();
        }
    }
    searchers.clear();
}

void GlobalNoteFinder::noteFound(const QString &noteId)
{
    NoteFinder *nf = dynamic_cast<NoteFinder *>(sender());
    emit        found(nf->storage().systemName(), noteId);
}

void GlobalNoteFinder::searcherFinished()
{
    NoteFinder *nf = dynamic_cast<NoteFinder *>(sender());
    searchers.remove(nf);
    nf->disconnect(this);
    if (!searchers.count()) {
        emit completed();
    }
}

/************************************************************************************
 * NoteManager                                                                      *
 ************************************************************************************/
NoteManager::NoteManager(QObject *parent) : QObject(parent)
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
    connect(storage.data(), SIGNAL(noteIdChanged(NoteListItem, QString)), SLOT(storageChanged()));

    storage->init();
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

void NoteManager::storageChanged() { emit storageChanged(_storages[((NoteStorage *)sender())->systemName()]); }

QList<NoteListItem> NoteManager::noteList(int count) const
{
    QList<NoteListItem> ret;
    foreach (NoteStorage::Ptr storage, prioritizedStorages()) {
        ret += storage->noteList(count);
    }
    std::sort(ret.begin(), ret.end(), noteListItemModifyComparer);
    return ret.mid(0, count);
}

Note NoteManager::note(const QString &storageId, const QString &noteId)
{
    if (!storageId.isEmpty() && !noteId.isEmpty()) {
        NoteStorage::Ptr s = storage(storageId);
        if (s) {
            return s->note(noteId);
        }
    }
    return Note();
}

const QMap<QString, NoteStorage::Ptr> NoteManager::storages(bool withInvalid) const
{
    if (withInvalid) {
        return _storages;
    }
    decltype(_storages) ret;
    for (auto &item : _storages) {
        if (item->isAccessible()) {
            ret.insert(item->systemName(), item);
        }
    }
    return ret;
}

const std::list<NoteStorage::Ptr> NoteManager::prioritizedStorages(bool withInvalid) const
{
    if (!_prioCache.size()) {

        auto storages = _storages;

        for (auto &code : _priorities) {
            auto storage = storages.take(code);
            if (storage) {
                _prioCache.push_back(storage);
            }
        }

        for (auto &storage : storages) {
            _prioCache.push_back(storage);
        }
    }

    if (withInvalid) {
        return _prioCache;
    }

    decltype(_prioCache) ret;
    for (auto &storage : _prioCache) {
        if (storage->isAccessible()) {
            ret.push_back(storage);
        }
    }

    return ret;
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

NoteManager *NoteManager::_instance = NULL;

} // namespace QtNote

#include "notemanager.moc"
