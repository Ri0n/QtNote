#ifndef NOTEWIDGET_H
#define NOTEWIDGET_H

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

#include "highlighterext.h"

namespace Ui {
class NoteWidget;
}

class TypeAheadFindBar;

namespace QtNote {

class NoteEdit;
class NoteHighlighter;
class Note;

class QTNOTE_EXPORT NoteWidget : public QWidget {
    Q_OBJECT

public:
    enum Feature { RichText = 1 };
    Q_DECLARE_FLAGS(Features, Feature)

    explicit NoteWidget(const QString &storageId, const QString &noteId);
    ~NoteWidget();

    void                    setText(QString text);
    void                    setNote(const Note &note);
    QString                 text();
    inline const Features  &features() const { return _features; }
    bool                    isMarkdown() const; // current mode. may look a little ugly
    inline void             setFeatures(const Features &features) { _features = features; }
    virtual NoteEdit       *editWidget() const;
    inline NoteHighlighter *highlighter() const { return _highlighter; }
    void                    setAcceptRichText(bool state);
    inline QString          storageId() const { return _storageId; }
    inline QString          noteId() const { return _noteId; }
    void                    setNoteId(const QString &noteId);
    inline const QString   &firstLine() const { return _firstLine; }
    inline qint64           lastChangeElapsed() const { return _lastChangeElapsed.elapsed(); }
    inline bool             isTrashRequested() const { return _trashRequested; }
    inline void             setTrashRequested(bool state) { _trashRequested = state; }
    void                    rehighlight();

signals:
    void firstLineChanged();
    void trashRequested();
    void saveRequested();
    void noteIdChanged(const QString &oldId, const QString &newId);
    void invalidated(); // emited when we are unsure we have the latest data

protected:
    void changeEvent(QEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool event(QEvent *event) override;
#endif

private:
    QAction *initAction(const char *icon, const QString &title, const QString &toolTip, const char *hotkey);

public slots:
    void save();
    void rereadSettings();

private slots:
    void onFindTriggered();
    void onReplaceTriggered();
    void autosave();
    void onCopyClicked();
    void textChanged();
    void onPrintClicked();
    void onSaveClicked();
    void onTrashClicked();
    void updateFirstLineColor();
    void focusReceived();

    void switchToText();
    void switchToMarkdown();

private:
    Ui::NoteWidget *ui = nullptr;

    QAction                              *mdModeAct;
    QAction                              *txtModeAct;
    TypeAheadFindBar                     *findBar      = nullptr;
    NoteHighlighter                      *_highlighter = nullptr;
    std::shared_ptr<HighlighterExtension> _linkHighlighter;
    QString                               _storageId;
    QString                               _noteId;
    QString                               _firstLine;
    QString                               _extFileName;
    QString                               _extSelecteFilter;
    QTimer                                _autosaveTimer;
    QElapsedTimer                         _lastChangeElapsed;
    Features                              _features;
    bool                                  _trashRequested = false;
    bool                                  _changed        = false;
};

} // namespace QtNote

Q_DECLARE_OPERATORS_FOR_FLAGS(QtNote::NoteWidget::Features)

#endif // NOTEWIDGET_H
