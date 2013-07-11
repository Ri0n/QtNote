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

#include <QStringList>

#include "notemanagermodel.h"
#include "notemanager.h"


class NMMItem
{
public:
	enum Type {
		Storage,
		Note
	};

	NMMItem(const StorageItem &storage)
		: parent(0)
		, type(Storage)
		, title(storage.storage->titleName())
		, id(storage.storage->systemName())
	{
		foreach (const NoteListItem &note, storage.storage->noteList()) {
			children.append(new NMMItem(note, this));
		}
	}

	NMMItem(const NoteListItem &note, NMMItem *parent)
		: parent(parent)
		, type(Note)
		, title(note.title)
		, id(note.id)
	{

	}

	~NMMItem()
	{
		qDeleteAll(children);
	}

	QIcon icon()
	{
		if (type == Storage) {
			return NoteManager::instance()->storage(id)->storageIcon();
		}
		return NoteManager::instance()->storage(parent->id)->noteIcon();
	}

	NMMItem *parent;
	Type type;
	QList<NMMItem *> children;
	QString title;
	QString id;
};

void debug(const QString &prefix, NMMItem *item)
{
	qDebug("%s: %s(%s)", qPrintable(prefix), item->type == NMMItem::Storage? "Storage":"Note", qPrintable(item->title));
}

NoteManagerModel::NoteManagerModel(QObject *parent)
	: QAbstractItemModel(parent)
{
	foreach (const StorageItem &s, NoteManager::instance()->storages()) {
		storages.append(new NMMItem(s));
		setStorageSignalHandlers(s.storage);
	}
	connect(NoteManager::instance(), SIGNAL(storageAdded(StorageItem)), SLOT(storageAdded(StorageItem)));
	connect(NoteManager::instance(), SIGNAL(storageRemoved(StorageItem)), SLOT(storageRemoved(StorageItem)));
}

NoteManagerModel::~NoteManagerModel()
{
	qDeleteAll(storages);
}

void NoteManagerModel::setStorageSignalHandlers(NoteStorage *s)
{
	connect(s, SIGNAL(noteAdded(NoteListItem)), SLOT(noteAdded(NoteListItem)));
	connect(s, SIGNAL(noteModified(NoteListItem)), SLOT(noteModified(NoteListItem)));
	connect(s, SIGNAL(noteRemoved(NoteListItem)), SLOT(noteRemoved(NoteListItem)));
	connect(s, SIGNAL(invalidated()), SLOT(storageInvalidated()));
}

QModelIndex NoteManagerModel::storageIndex(const QString &storageId) const
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

QModelIndex NoteManagerModel::noteIndex(const QString &storageId, const QString &noteId) const
{
	QModelIndex storage = storageIndex(storageId);
	if (storage.isValid()) {
		NMMItem *storageItem = static_cast<NMMItem*>(storage.internalPointer());
		int i = 0;
		foreach (NMMItem *c, storageItem->children) {
			if (c->id == noteId) {
				return index(i, 0, storage);
			}
			i++;
		}
	}
	return QModelIndex();
}

void NoteManagerModel::storageAdded(const StorageItem &)
{

}

void NoteManagerModel::storageRemoved(const StorageItem &)
{

}

void NoteManagerModel::storageInvalidated()
{
	/* just a hack. actually we should remove all items of this storage */
	beginResetModel();
	endResetModel();
}

void NoteManagerModel::noteAdded(const NoteListItem &item)
{
	QModelIndex parentIndex = storageIndex(item.storageId);
	if (parentIndex.isValid()) {
		NMMItem *storage = static_cast<NMMItem*>(parentIndex.internalPointer());
		int len = rowCount(parentIndex);
		beginInsertRows(parentIndex, len, len);
		storage->children.append(new NMMItem(item, storage));
		endInsertRows();
	}
}

void NoteManagerModel::noteModified(const NoteListItem &note)
{
	QModelIndex index = noteIndex(note.storageId, note.id);
	if (index.isValid()) {
		NMMItem *noteItem = static_cast<NMMItem*>(index.internalPointer());
		noteItem->title = note.title;
		emit dataChanged(index, index);
	}
}

void NoteManagerModel::noteRemoved(const NoteListItem &item)
{
	QModelIndex index = noteIndex(item.storageId, item.id);
	if (index.isValid()) {
		removeRow(index.row(), index.parent());
	}
}


QModelIndex NoteManagerModel::index( int row, int column,
									 const QModelIndex & parent ) const
{
	if (!hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	if (column < 1) {
		if (parent.isValid()) {
			NMMItem *parentItem = static_cast<NMMItem *>(parent.internalPointer());
			if (parentItem->type == NMMItem::Storage && row < parentItem->children.count()) {
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

QModelIndex NoteManagerModel::parent( const QModelIndex & index ) const
{
	if (index.isValid()) {
		NMMItem *item = static_cast<NMMItem *>(index.internalPointer());
		int n = storages.indexOf(item);
		if (n == -1) { // its valid note
			return createIndex(storages.indexOf(item->parent), 0, item->parent);
		}
	}
	return QModelIndex();
}

int NoteManagerModel::rowCount( const QModelIndex & parent ) const
{
	if (parent.isValid()) {
		return static_cast<NMMItem *>(parent.internalPointer())->children.count();
	} else {
		return storages.size();
	}
	return 0;
}

int NoteManagerModel::columnCount( const QModelIndex &parent) const
{
	if (parent.isValid() && static_cast<NMMItem *>(parent.internalPointer())->type == NMMItem::Note) { // note has no children so 0 columns
		return 0;
	}
	return 1;
}

QVariant NoteManagerModel::data( const QModelIndex & index, int role ) const
{
	if (index.isValid()) {
		NMMItem *item = static_cast<NMMItem *>(index.internalPointer());
		switch (role) {
			case Qt::DisplayRole:
				return item->title;
			case Qt::DecorationRole:
				return item->icon();
			case StorageId:
				if (item->type == NMMItem::Storage) {
					return item->id;
				}
				return item->parent->id;
			case NoteId:
				if (item->type == NMMItem::Storage) {
					return "";
				}
				return item->id;
		}
	}
	return QVariant();
}

bool NoteManagerModel::removeRows(int row, int count, const QModelIndex &parent)
{
	QList<NMMItem*> *source;
	if (parent.isValid()) { // note
		NMMItem *storageItem = static_cast<NMMItem *>(parent.internalPointer());
		source = &storageItem->children;
	} else { //storage
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
