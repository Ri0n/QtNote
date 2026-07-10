#ifndef NOTEWIDGET_H
#define NOTEWIDGET_H

#include <QElapsedTimer>
#include <QIcon>
#include <QPointer>
#include <QTimer>
#include <QWidget>

#include "highlighterext.h"
#include "note.h"

namespace Ui {
class NoteWidget;
}

class TypeAheadFindBar;
class QAction;
class QToolButton;

namespace QtNote {

class NoteEdit;
class NoteHighlighter;
class SpeechAudioRecorder;
class SpeechRecognitionJob;
class SpeechRecognitionProviderInterface;

class QTNOTE_EXPORT NoteWidget : public QWidget {
    Q_OBJECT

public:
    enum Feature { RichText = 1 };
    Q_DECLARE_FLAGS(Features, Feature)

    explicit NoteWidget(const Note &note = {});
    ~NoteWidget();

    void              setText(QString text);
    void              setNote(const Note &note);
    QString           text();
    const Features   &features() const { return _features; }
    bool              isMarkdown() const; // current mode. may look a little ugly
    void              setFeatures(const Features &features) { _features = features; }
    virtual NoteEdit *editWidget() const;
    NoteHighlighter  *highlighter() const { return _highlighter; }
    void              setAcceptRichText(bool state);
    void              setSpeechRecognitionProvider(SpeechRecognitionProviderInterface *provider);
    Note              note() const { return _note; }
    QString           storageId() const { return _note.storageId(); }
    QString           noteId() const { return _note.id(); }
    const QString    &firstLine() const { return _firstLine; }
    qint64            lastChangeElapsed() const { return _lastChangeElapsed.elapsed(); }
    bool              isTrashRequested() const { return _trashRequested; }
    void              setTrashRequested(bool state) { _trashRequested = state; }
    void              rehighlight();

signals:
    void firstLineChanged();
    void trashRequested();
    void noteIdChanged(const QString &oldId, const QString &newId);

protected:
    void changeEvent(QEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool event(QEvent *event) override;
#endif

private:
    void     initFromNote();
    void     setContents(const QString &title, const QString &body, Note::Format format);
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
    void startSpeechRecognition();
    void finishSpeechRecognition();
    void cancelSpeechRecognition();
    void updateSpeechRecognitionProgress(qint64 elapsedMs, qint64 maxDurationMs);

private:
    void    updateSpeechRecognitionAction();
    void    showSpeechRecognitionError(const QString &error);
    void    appendRecognizedText(const QString &text);
    QString speechRecognitionLanguage() const;
    QString normalizeSpeechRecognitionLanguage(const QString &language) const;
    QString speechContextId() const;

    Ui::NoteWidget *ui = nullptr;

    QAction                              *mdModeAct;
    QAction                              *txtModeAct;
    QAction                              *speechAction = nullptr;
    QIcon                                 speechIdleIcon;
    QToolButton                          *speechButton   = nullptr;
    SpeechRecognitionProviderInterface   *speechProvider = nullptr;
    SpeechAudioRecorder                  *speechRecorder = nullptr;
    QPointer<SpeechRecognitionJob>        speechJob;
    QString                               localSpeechContextId;
    TypeAheadFindBar                     *findBar      = nullptr;
    NoteHighlighter                      *_highlighter = nullptr;
    std::shared_ptr<HighlighterExtension> _linkHighlighter;
    Note                                  _note;
    QString                               _firstLine;
    QString                               _extFileName;
    QString                               _extSelecteFilter;
    QTimer                                _autosaveTimer;
    QElapsedTimer                         _lastChangeElapsed;
    Features                              _features;
    bool                                  _trashRequested       = false;
    bool                                  _changed              = false;
    bool                                  speechRecognitionBusy = false;
};

} // namespace QtNote

Q_DECLARE_OPERATORS_FOR_FLAGS(QtNote::NoteWidget::Features)

#endif // NOTEWIDGET_H
