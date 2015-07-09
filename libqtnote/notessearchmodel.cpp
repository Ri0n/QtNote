#include "notessearchmodel.h"

namespace QtNote {

NotesSearchModel::NotesSearchModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{

}

bool NotesSearchModel::filterAcceptsRow(int sourceRow,
                                        const QModelIndex &sourceParent) const
{
    if (!sourceParent.isValid()) {
        return true; // include storages
    }
    // TODO handle search in text
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void NotesSearchModel::setSearchInText(bool allow)
{
    // TODO implement
}

} // namespace QtNote

