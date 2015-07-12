#include "notessearchmodel.h"
#include "notemanager.h"
#include "notesmodel.h"

namespace QtNote {

NotesSearchModel::NotesSearchModel(QObject *parent) :
    QSortFilterProxyModel(parent),
    _searchInBody(false)
{
    _finder = NoteManager::search();
    _finder->setParent(this);
    connect(_finder, SIGNAL(found(QString,QString)), SLOT(noteFound(QString,QString)));
}

void NotesSearchModel::setSearchText(const QString &text)
{
    _text = text;
    _foundCache.clear();
    if (_searchInBody) {
        _finder->start(_text);
    }
    setFilterFixedString(text);
}

bool NotesSearchModel::filterAcceptsRow(int sourceRow,
                                        const QModelIndex &sourceParent) const
{
    if (!sourceParent.isValid()) {
        return true; // include storages
    }
    if (_searchInBody) {
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        QString storageId = sourceModel()->data(index, NotesModel::StorageIdRole).toString();
        if (_foundCache.contains(storageId)) {
            QString noteId = sourceModel()->data(index, NotesModel::NoteIdRole).toString();
            if (_foundCache[storageId].contains(noteId)) {
                return true;
            }
        }
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void NotesSearchModel::setSearchInBody(bool allow)
{
    _searchInBody = allow;
    if (allow) {
        _finder->start(_text);
    } else {
        _finder->abort();
        if (_foundCache.count()) {
            _foundCache.clear();
            invalidateFilter();
        }
    }
}

void NotesSearchModel::noteFound(const QString &storageId, const QString &noteId)
{
    if (!_foundCache.contains(storageId)) {
        _foundCache[storageId] = QStringList(noteId);
    } else {
        _foundCache[storageId] << noteId;
    }
    static_cast<NotesModel*>(sourceModel())->invalidateNote(storageId, noteId);
}

} // namespace QtNote

