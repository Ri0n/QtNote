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
	explicit NoteDialog(QWidget *parent, const QString &storageId, const QString &noteId);
    virtual ~NoteDialog();
	void setText(QString text);
	QString text();
	bool checkOwnership(const QString &storageId, const QString &noteId) const;

protected:
    virtual void changeEvent(QEvent *e);

private:
    Ui::NoteDialog *m_ui;
	QString storageId_;
	QString noteId_;
	bool trashRequested_;

signals:
	void trashRequested(const QString &, const QString &);
	void saveRequested(const QString &, const QString &, const QString &);

public slots:
	void done(int r);

private slots:
	void trashClicked();
};

#endif // NOTEDIALOG_H
