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

#include <QIODevice>
#include <QMimeData>
#include <QStringList>

#include <algorithm>

#include "notemanager.h"
#include "notesmodel.h"

namespace QtNote {

class NMMItem {
public:
    NMMItem(const NoteStorage::Ptr &storage) :
        parent(0), type(NotesModel::ItemStorage), title(storage->name()), id(storage->systemName())
    {
    }

    NMMItem(const Note &note, NMMItem *parent) :
        parent(parent), type(NotesModel::ItemNote), title(note.displayTitle()), id(note.id()), tags(note.tags()),
        lastChange(note.lastChangeUTC())
    {
    }

    ~NMMItem() { qDeleteAll(children); }

    void sortChildren()
    {
        std::sort(children.begin(), children.end(),
                  [](const NMMItem *a, const NMMItem *b) { return a->lastChange > b->lastChange; });
    }

    int insertPosition(const Note &note) const
    {
        const auto lastChange = note.lastChangeUTC();
        for (int i = 0; i < children.size(); ++i) {
            if (lastChange > children.at(i)->lastChange) {
                return i;
            }
        }
        return children.size();
    }

    QIcon icon()
    {
        if (type == NotesModel::ItemStorage) {
            return NoteManager::instance()->storage(id)->storageIcon();
        }
        return NoteManager::instance()->storage(parent->id)->noteIcon();
    }

    NMMItem             *parent;
    NotesModel::ItemType type;
    QList<NMMItem *>     children;
    QString              title;
    QString              id;
    QStringList          tags;
    QDateTime            lastChange;
};

void debug(const QString &prefix, NMMItem *item)
{
    qDebug("%s: %s(%s)", qPrintable(prefix), item->type == NotesModel::ItemStorage ? "Storage" : "Note",
           qPrintable(item->title));
}

NotesModel::NotesModel(QObject *parent) : QAbstractItemModel(parent)
{
    foreach (const NoteStorage::Ptr &s, NoteManager::instance()->storages(true)) {
        storages.append(new NMMItem(s));
        setStorageSignalHandlers(s);
        refreshStorage(s);
    }
    connect(NoteManager::instance(), SIGNAL(storageAdded(NoteStorage::Ptr)), SLOT(storageAdded(NoteStorage::Ptr)));
    connect(NoteManager::instance(), SIGNAL(storageRemoved(NoteStorage::Ptr)), SLOT(storageRemoved(NoteStorage::Ptr)));
}

NotesModel::~NotesModel() { qDeleteAll(storages); }

void NotesModel::setStorageSignalHandlers(NoteStorage::Ptr s)
{
    connect(s.data(), SIGNAL(noteAdded(Note)), SLOT(noteAdded(Note)));
    connect(s.data(), SIGNAL(noteModified(Note)), SLOT(noteModified(Note)));
    connect(s.data(), SIGNAL(noteRemoved(Note)), SLOT(noteRemoved(Note)));
    connect(s.data(), SIGNAL(invalidated()), SLOT(storageInvalidated()));
}

void NotesModel::invalidateNote(const QString &storageId, const QString &noteId)
{
    auto index = noteIndex(storageId, noteId);
    if (index.isValid()) {
        dataChanged(index, index);
    }
}

QModelIndex NotesModel::storageIndex(const QString &storageId) const
{
    int i = 0;
    foreach (NMMItem *item, storages) {
        if (item->id == storageId) {
            break;
        }
        i++;
    }
    return index(i, 0, QModelIndex());
}

QModelIndex NotesModel::noteIndex(const QString &storageId, const QString &noteId) const
{
    QModelIndex storage = storageIndex(storageId);
    if (storage.isValid()) {
        NMMItem *storageItem = static_cast<NMMItem *>(storage.internalPointer());
        int      i           = 0;
        foreach (NMMItem *c, storageItem->children) {
            if (c->id == noteId) {
                return index(i, 0, storage);
            }
            i++;
        }
    }
    return QModelIndex();
}

void NotesModel::storageAdded(const NoteStorage::Ptr &storage)
{
    beginInsertRows({}, storages.size(), storages.size());
    storages.append(new NMMItem(storage));
    endInsertRows();
    setStorageSignalHandlers(storage);
    refreshStorage(storage);
    emit statsChanged();
}

void NotesModel::storageRemoved(const NoteStorage::Ptr &)
{
    // TODO implement
    emit statsChanged();
}

void NotesModel::storageInvalidated()
{
    auto storage = NoteManager::instance()->storage(static_cast<NoteStorage *>(sender())->systemName());
    if (storage)
        refreshStorage(storage);
}

void NotesModel::refreshStorage(const NoteStorage::Ptr &storage)
{
    auto *job = storage->refreshNotesAsync(0, this);
    connect(job, &StorageJob::finished, this, [this, storage, job]() {
        if (!storage || job->state() != StorageJob::Succeeded)
            return;
        const auto parentIndex = storageIndex(storage->systemName());
        if (!parentIndex.isValid())
            return;
        auto *item = static_cast<NMMItem *>(parentIndex.internalPointer());
        beginResetModel();
        qDeleteAll(item->children);
        item->children.clear();
        for (const auto &note : job->result())
            item->children.append(new NMMItem(note, item));
        item->sortChildren();
        endResetModel();
        emit statsChanged();
    });
}

void NotesModel::noteAdded(const Note &item)
{
    QModelIndex parentIndex = storageIndex(item.storage()->systemName());
    if (parentIndex.isValid()) {
        NMMItem *storage = static_cast<NMMItem *>(parentIndex.internalPointer());
        int      pos     = storage->insertPosition(item);
        beginInsertRows(parentIndex, pos, pos);
        storage->children.insert(pos, new NMMItem(item, storage));
        endInsertRows();
        emit statsChanged();
    }
}

void NotesModel::noteModified(const Note &note)
{
    QModelIndex index = noteIndex(note.storage()->systemName(), note.id());
    if (index.isValid()) {
        NMMItem *noteItem    = static_cast<NMMItem *>(index.internalPointer());
        noteItem->title      = note.displayTitle();
        noteItem->tags       = note.tags();
        noteItem->lastChange = note.lastChangeUTC();
        emit layoutAboutToBeChanged();
        noteItem->parent->sortChildren();
        emit layoutChanged();
        index = noteIndex(note.storage()->systemName(), note.id());
        emit dataChanged(index, index);
    }
}

void NotesModel::noteRemoved(const Note &item)
{
    QModelIndex index = noteIndex(item.storage()->systemName(), item.id());
    if (index.isValid()) {
        removeRow(index.row(), index.parent());
        emit statsChanged();
    }
}

QModelIndex NotesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    if (column < 1) {
        if (parent.isValid()) {
            NMMItem *parentItem = static_cast<NMMItem *>(parent.internalPointer());
            if (parentItem->type == ItemStorage && row < parentItem->children.count()) {
                return createIndex(row, column, parentItem->children[row]);
            }
        } else {
            if (row < storages.count()) {
                return createIndex(row, column, storages[row]);
            }
        }
    }
    return QModelIndex();
}

QModelIndex NotesModel::parent(const QModelIndex &index) const
{
    if (index.isValid()) {
        NMMItem *item = static_cast<NMMItem *>(index.internalPointer());
        int      n    = storages.indexOf(item);
        if (n == -1) { // its valid note
            return createIndex(storages.indexOf(item->parent), 0, item->parent);
        }
    }
    return QModelIndex();
}

int NotesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return static_cast<NMMItem *>(parent.internalPointer())->children.count();
    } else {
        return storages.size();
    }
    return 0;
}

int NotesModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()
        && static_cast<NMMItem *>(parent.internalPointer())->type == ItemNote) { // note has no children so 0 columns
        return 0;
    }
    return 1;
}

Qt::ItemFlags NotesModel::flags(const QModelIndex &index) const
{
    if (index.isValid()) {
        NMMItem *parentItem = static_cast<NMMItem *>(index.internalPointer());
        if (parentItem->type == ItemStorage) {
            return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
        } else { // note
            return Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable;
        }
    }
    return QAbstractItemModel::flags(index);
}

QVariant NotesModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        NMMItem *item = static_cast<NMMItem *>(index.internalPointer());
        switch (role) {
        case Qt::DisplayRole:
            return item->title;
        case Qt::DecorationRole:
            return item->icon();
        case ItemTypeRole:
            return item->type;
        case StorageIdRole:
            if (item->type == ItemStorage) {
                return item->id;
            }
            return item->parent->id;
        case NoteIdRole:
            if (item->type == ItemStorage) {
                return "";
            }
            return item->id;
        case TagsRole:
            if (item->type == ItemStorage) {
                return QStringList();
            }
            return item->tags;
        }
    }
    return QVariant();
}

bool NotesModel::removeRows(int row, int count, const QModelIndex &parent)
{
    QList<NMMItem *> *source;
    if (parent.isValid()) { // note
        if (removeProtection == parent) {
            removeProtection = QModelIndex();
            return false;
        }
        NMMItem *storageItem = static_cast<NMMItem *>(parent.internalPointer());
        source               = &storageItem->children;
    } else { // storage
        source = &storages;
    }
    if (row < source->count() && count > 0) {
        beginRemoveRows(parent, row, row + count - 1);
        count = (row + count) > source->count() ? source->count() - row : count;

        for (int i = 0; i < count; i++) {
            delete source->takeAt(row);
        }

        endRemoveRows();
        return true;
    }

    return false;
}

Qt::DropActions NotesModel::supportedDropActions() const { return Qt::CopyAction; }

QStringList NotesModel::mimeTypes() const { return QStringList() << "application/qtnote.notes.list"; }

QMimeData *NotesModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    foreach (const QModelIndex &index, indexes) {
        if (index.isValid()) {
            NMMItem *item = static_cast<NMMItem *>(index.internalPointer());
            // only note items are draggable, so do not check it here
            stream << item->parent->id << item->id << item->title;
        }
    }

    mimeData->setData("application/qtnote.notes.list", encodedData);
    return mimeData;
}

bool NotesModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                              const QModelIndex &parent)
{
    Q_UNUSED(row)

    if (action == Qt::IgnoreAction)
        return true;

    if (!data->hasFormat("application/qtnote.notes.list"))
        return false;

    if (column > 0)
        return false;

    if (!parent.isValid())
        return false;

    NMMItem *dstStorageItem = static_cast<NMMItem *>(parent.internalPointer());
    if (dstStorageItem->type != ItemStorage)
        return false;
    auto dstStorage  = NoteManager::instance()->storage(dstStorageItem->id);
    removeProtection = parent;

    QByteArray  encodedData = data->data("application/qtnote.notes.list");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    while (!stream.atEnd()) {
        QString storageId, noteId, title;
        stream >> storageId >> noteId >> title;
        if (dstStorageItem->id == storageId) {
            continue;
        }
        auto srcStorage = NoteManager::instance()->storage(storageId);
        Note note       = srcStorage->note(noteId);
        if (!note.isEmpty()) {
            if (dstStorage->availableFormats().contains(note.format())) {
                auto newNote = dstStorage->createNote();
                newNote.setTitle(note.title());
                newNote.setText(note.text(), note.format());
                if (newNote.save()) {
                    note.remove();
                }
            }
        }
    }

    return true;
}

} // namespace QtNote
