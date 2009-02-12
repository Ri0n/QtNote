#ifndef NOTEDIALOG_H
#define NOTEDIALOG_H

#include <QtGui/QDialog>

namespace Ui {
    class NoteDialog;
}

class NoteDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(NoteDialog)
public:
	explicit NoteDialog(QWidget *parent = 0);
    virtual ~NoteDialog();
	void setText(QString text);
	QString text();

protected:
    virtual void changeEvent(QEvent *e);

private:
    Ui::NoteDialog *m_ui;
};

#endif // NOTEDIALOG_H
