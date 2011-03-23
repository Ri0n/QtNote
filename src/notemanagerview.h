#ifndef NOTEMANAGERVIEW_H
#define NOTEMANAGERVIEW_H

#include <QTreeView>

class NoteManagerView : public QTreeView
{
    Q_OBJECT
public:
    explicit NoteManagerView(QWidget *parent = 0);

protected:
	// reimplemented
	void contextMenuEvent(QContextMenuEvent *e);

signals:

public slots:
	void removeSelected();

};

#endif // NOTEMANAGERVIEW_H
