#ifndef NOTESSEARCHMODEL_H
#define NOTESSEARCHMODEL_H

#include <QSortFilterProxyModel>
#include <QHash>
#include <QStringList>

namespace QtNote {

class GlobalNoteFinder;

class NotesSearchModel : public QSortFilterProxyModel
{
    Q_OBJECT

    bool _searchInBody;
    QString _text;
    GlobalNoteFinder *_finder;
    QHash<QString,QStringList> _foundCache;

public:
    NotesSearchModel(QObject *parent = 0);
public slots:
    void setSearchText(const QString &text);
    void setSearchInBody(bool allow);
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
private slots:
    void noteFound(const QString &storageId, const QString &noteId);
};

} // namespace QtNote

#endif // NOTESSEARCHMODEL_H
