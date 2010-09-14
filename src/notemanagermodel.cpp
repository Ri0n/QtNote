#include "notemanagermodel.h"
#include "notemanager.h"

NoteManagerModel::NoteManagerModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

QModelIndex NoteManagerModel::index( int row, int column,
									 const QModelIndex & parent ) const
{
	if (parent.isValid()) {

	} else {
		if (column == 0 && row < NoteManager::instance()->storages().size()) {
			return createIndex(row, column,
							   qHash(NoteManager::instance()->storages().keys()[row]));
		}
	}
	return QModelIndex();
}

QModelIndex NoteManagerModel::parent( const QModelIndex & index ) const
{
	return QModelIndex();
}

int NoteManagerModel::rowCount( const QModelIndex & parent ) const
{
	if (parent.isValid()) {

	} else {
		return NoteManager::instance()->storages().size();
	}
	return 0;
}

int NoteManagerModel::columnCount( const QModelIndex & parent ) const
{
	return 1;
}

QVariant NoteManagerModel::data( const QModelIndex & index, int role ) const
{
	if (role == Qt::DisplayRole && index.isValid()) {
		QList<StorageItem> storageItems = NoteManager::instance()->storages().values();
		if (index.row() < storageItems.size() && index.column() == 0) {
			return storageItems[index.row()].storage->titleName();
		}
	}
	return QVariant();
}
