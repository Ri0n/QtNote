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

#ifndef NOTEMANAGERMODEL_H
#define NOTEMANAGERMODEL_H

#include <QAbstractItemModel>

class NoteStorage;
class NMMItem;
struct StorageItem;
struct NoteListItem;

class NoteManagerModel : public QAbstractItemModel
{
    Q_OBJECT
public:
	enum DataRole {
		StorageId = Qt::UserRole + 1,
		NoteId
	};

	explicit NoteManagerModel(QObject *parent = 0);
	~NoteManagerModel();
	void setStorageSignalHandlers(NoteStorage *s);

	QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
	QModelIndex parent( const QModelIndex & index ) const;
	int rowCount( const QModelIndex & parent = QModelIndex() ) const;
	int columnCount( const QModelIndex & parent = QModelIndex() ) const;
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

signals:

public slots:

private slots:
	void storageAdded(const StorageItem &);
	void storageRemoved(const StorageItem &);
	void noteAdded(const NoteListItem &);
	void noteModified(const NoteListItem &);
	void noteRemoved(const NoteListItem &);

private:
	QModelIndex storageIndex(const QString &) const;
	QModelIndex noteIndex(const QString &, const QString &) const;

private:
	QList<NMMItem*> storages;
};

#endif // NOTEMANAGERMODEL_H
