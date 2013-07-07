#ifndef NOTEDIALOGEDIT_H
#define NOTEDIALOGEDIT_H

#include <QTextEdit>

class QDropEvent;

class NoteDialogEdit : public QTextEdit
{
	Q_OBJECT
public:
	explicit NoteDialogEdit(QWidget *parent = 0);

protected:
	void dropEvent(QDropEvent *e);

signals:
	
public slots:
	
};

#endif // NOTEDIALOGEDIT_H
