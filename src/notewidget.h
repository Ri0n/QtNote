#ifndef NOTEWIDGET_H
#define NOTEWIDGET_H

#include <QWidget>
#include <QTimer>

namespace Ui {
class NoteWidget;
}

class TypeAheadFindBar;

class NoteWidget : public QWidget
{
	Q_OBJECT
	
public:
	explicit NoteWidget(const QString &storageId, const QString &noteId);
	~NoteWidget();

	void setText(QString text);
	QString text();
	void setAcceptRichText(bool state);
	inline QString storageId() const { return storageId_; }
	inline QString noteId() const { return noteId_; }
	void setNoteId(const QString &noteId);
	inline void save() { if (changed_) emit saveRequested(); }
	inline const QString &firstLine() const { return firstLine_; }

signals:
	void firstLineChanged();
	void trashRequested();
	void saveRequested();
	void noteIdChanged(const QString &oldId, const QString &newId);

protected:
	void changeEvent(QEvent *e);
	void keyPressEvent(QKeyEvent *event);

private slots:
	void autosave();
	void copyClicked();
	void textChanged();
	void on_printBtn_clicked();
	void on_saveBtn_clicked();
	void on_trashBtn_clicked();

private:
	Ui::NoteWidget *ui;

	TypeAheadFindBar *findBar;
	QString storageId_;
	QString noteId_;
	QString firstLine_;
	QString extFileName_;
	QTimer autosaveTimer_;
	bool trashRequested_;
	bool changed_;
};

#endif // NOTEWIDGET_H
