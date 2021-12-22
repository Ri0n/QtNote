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

#ifndef NOTESMODEL_H
#define NOTESMODEL_H

#include "notestorage.h"
#include <QAbstractItemModel>

namespace QtNote {

class NMMItem;
struct NoteListItem;

class NotesModel : public QAbstractItemModel {
    Q_OBJECT
public:
    enum DataRole { StorageIdRole = Qt::UserRole + 1, NoteIdRole, ItemTypeRole };

    enum ItemType { ItemStorage, ItemNote };

    explicit NotesModel(QObject *parent = 0);
    ~NotesModel();
    void setStorageSignalHandlers(NoteStorage::Ptr s);
    void invalidateNote(const QString &storageId, const QString &noteId);

    QModelIndex   index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex   parent(const QModelIndex &index) const;
    int           rowCount(const QModelIndex &parent = QModelIndex()) const;
    int           columnCount(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant      data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool          removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

    Qt::DropActions supportedDropActions() const;
    QStringList     mimeTypes() const;
    QMimeData      *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

signals:
    void statsChanged(); // emit on note added/removed, storage added/removed

public slots:

private slots:
    void storageAdded(const NoteStorage::Ptr &);
    void storageRemoved(const NoteStorage::Ptr &);
    void noteAdded(const NoteListItem &);
    void noteModified(const NoteListItem &);
    void noteRemoved(const NoteListItem &);

    void storageInvalidated();

private:
    QModelIndex storageIndex(const QString &) const;
    QModelIndex noteIndex(const QString &, const QString &) const;

private:
    QList<NMMItem *> storages;
    QModelIndex      removeProtection;
};

} // namespace QtNote

#endif // NOTESMODEL_H
