#ifndef NOTESSEARCHMODEL_H
#define NOTESSEARCHMODEL_H

#include <QSortFilterProxyModel>

namespace QtNote {

class NotesSearchModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    NotesSearchModel(QObject *parent = 0);
public slots:
    void setSearchInText(bool allow);
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
};

} // namespace QtNote

#endif // NOTESSEARCHMODEL_H
