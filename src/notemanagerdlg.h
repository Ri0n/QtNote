#ifndef NOTEMANAGERDLG_H
#define NOTEMANAGERDLG_H

#include <QDialog>
#include <QModelIndex>

class NoteManagerModel;

namespace Ui {
    class NoteManagerDlg;
}

class NoteManagerDlg : public QDialog
{
    Q_OBJECT

public:
    explicit NoteManagerDlg(QWidget *parent = 0);
    ~NoteManagerDlg();

protected:
    void changeEvent(QEvent *e);

private slots:
	void itemDoubleClicked(const QModelIndex &index);

private:
    Ui::NoteManagerDlg *ui;
	NoteManagerModel *model;
};

#endif // NOTEMANAGERDLG_H
