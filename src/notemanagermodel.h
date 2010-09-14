#ifndef NOTEMANAGERMODEL_H
#define NOTEMANAGERMODEL_H

#include <QAbstractItemModel>

class NoteManagerModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit NoteManagerModel(QObject *parent = 0);
	QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
	QModelIndex parent( const QModelIndex & index ) const;
	int rowCount( const QModelIndex & parent = QModelIndex() ) const;
	int columnCount( const QModelIndex & parent = QModelIndex() ) const;
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

signals:

public slots:

};

#endif // NOTEMANAGERMODEL_H
