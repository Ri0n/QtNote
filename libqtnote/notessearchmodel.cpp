#include "notessearchmodel.h"
#include "notemanager.h"
#include "notesmodel.h"

#include <QtGlobal>

namespace QtNote {

NotesSearchModel::NotesSearchModel(QObject *parent) : QSortFilterProxyModel(parent), _searchInBody(false)
{
    _finder = NoteManager::search();
    _finder->setParent(this);
    connect(_finder, SIGNAL(found(QString, QString)), SLOT(noteFound(QString, QString)));
}

bool NotesSearchModel::hasBodyMatch(const QString &storageId, const QString &noteId) const
{
    return _foundCache.contains(storageId) && _foundCache[storageId].contains(noteId);
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

bool NotesSearchModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!sourceParent.isValid()) {
        return true; // include storages
    }
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    if (_searchInBody) {
        QString storageId = sourceModel()->data(index, NotesModel::StorageIdRole).toString();
        if (_foundCache.contains(storageId)) {
            QString noteId = sourceModel()->data(index, NotesModel::NoteIdRole).toString();
            if (_foundCache[storageId].contains(noteId)) {
                return true;
            }
        }
    }
    if (QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent)) {
        return true;
    }

    auto filter = _text.trimmed();
    if (filter.startsWith(QLatin1Char('*'))) {
        filter.remove(0, 1);
    }
    if (filter.isEmpty()) {
        return false;
    }

    const auto tags = sourceModel()->data(index, NotesModel::TagsRole).toStringList();
    for (const auto &tag : tags) {
        if (tag.contains(filter, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
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
            invalidateRowsFilter();
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
    static_cast<NotesModel *>(sourceModel())->invalidateNote(storageId, noteId);
}

void NotesSearchModel::invalidateRowsFilter()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateFilter();
#endif
}

} // namespace QtNote
