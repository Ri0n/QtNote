#ifndef NOTEWIDGET_H
#define NOTEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>

namespace Ui {
class NoteWidget;
}

class TypeAheadFindBar;

namespace QtNote {

struct ActData;
class NoteDialogEdit;

class NoteWidget : public QWidget
{
	Q_OBJECT
	
public:
	explicit NoteWidget(const QString &storageId, const QString &noteId);
	~NoteWidget();

	void setText(QString text);
	QString text();
	NoteDialogEdit* editWidget() const;
	void setAcceptRichText(bool state);
	inline QString storageId() const { return _storageId; }
	inline QString noteId() const { return noteId_; }
	void setNoteId(const QString &noteId);
	inline const QString &firstLine() const { return _firstLine; }
	inline qint64 lastChangeElapsed() const { return _lastChangeElapsed.elapsed(); }
	inline bool isTrashRequested() const { return _trashRequested; }
	inline void setTrashRequested(bool state) { _trashRequested = state; }

signals:
	void firstLineChanged();
	void trashRequested();
	void saveRequested();
	void noteIdChanged(const QString &oldId, const QString &newId);
	void invalidated(); // emited when we are unsure we have the latest data

protected:
	void changeEvent(QEvent *e);
	void keyPressEvent(QKeyEvent *event);	

public slots:
	void save();

private slots:
	void onFindTriggered();
	void onReplaceTriggered();
	void autosave();
	void onCopyClicked();
	void textChanged();
	void onPrintClicked();
	void onSaveClicked();
	void onTrashClicked();

private:
	Ui::NoteWidget *ui;

	TypeAheadFindBar *findBar;
	QString _storageId;
	QString noteId_;
	QString _firstLine;
	QString extFileName_;
	QTimer _autosaveTimer;
	QElapsedTimer _lastChangeElapsed;
	bool _trashRequested;
	bool _changed;

	QAction *initAction(const ActData &name);
};

} // namespace QtNote

#endif // NOTEWIDGET_H
