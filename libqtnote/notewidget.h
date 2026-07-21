#ifndef NOTEWIDGET_H
#define NOTEWIDGET_H

#include <QElapsedTimer>
#include <QIcon>
#include <QPointer>
#include <QTimer>
#include <QUuid>
#include <QWidget>

#include "highlighterext.h"
#include "note.h"

namespace Ui {
class NoteWidget;
}

class TypeAheadFindBar;
class QAction;
class QToolButton;
class QImage;

namespace QtNote {

class NoteEdit;
class NoteHighlighter;
class QmlNoteEditor;
class SpeechAudioRecorder;
class SpeechRecognitionJob;
class NoteSaveJob;
class SpeechRecognitionProviderInterface;
struct DraftRecord;

class QTNOTE_EXPORT NoteWidget : public QWidget {
    Q_OBJECT

public:
    enum Feature { RichText = 1 };
    Q_DECLARE_FLAGS(Features, Feature)

    explicit NoteWidget(const Note &note = {}, const QUuid &draftId = {});
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
    QUuid             draftId() const { return _draftId; }
    const QString    &firstLine() const { return _firstLine; }
    qint64            lastChangeElapsed() const { return _lastChangeElapsed.elapsed(); }
    bool              isTrashRequested() const { return _trashRequested; }
    bool              hasPersistedDraft() const { return _draftPersisted; }
    void              setTrashRequested(bool state) { _trashRequested = state; }
    void              setStickyNotesAvailable(bool available);
    void              rehighlight();
    void              addHighlightExtension(const std::shared_ptr<HighlighterExtension> &extension, int type);
    void              findText(const QString &text, bool focusFindBar = true);
    bool              prepareToClose();
    void              discardDraft();

signals:
    void firstLineChanged();
    void trashRequested();
    void noteIdChanged(const QString &oldId, const QString &newId);
    void pinRequested();

protected:
    void changeEvent(QEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool event(QEvent *event) override;
#endif

private:
    enum class ContentLoadPolicy { ResetHistory, RecordFormatConversion };

    void     initFromNote();
    void     setContents(const QString &title, const QString &body, Note::Format format,
                         ContentLoadPolicy policy = ContentLoadPolicy::ResetHistory);
    void     loadMediaResources();
    void     resizeMediaToViewport();
    void     adoptEditingDraft(const DraftRecord &draft);
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
    void insertImage();
    void insertClipboardImage(const QImage &image);
    void finishSpeechRecognition();
    void cancelSpeechRecognition();
    void updateSpeechRecognitionProgress(qint64 elapsedMs, qint64 maxDurationMs);

private:
    void    updateSpeechRecognitionAction();
    void    syncEditorMode(bool markdown);
    void    insertTableBlock();
    void    insertListBlock(int type);
    void    showSpeechRecognitionError(const QString &error);
    void    appendRecognizedText(const QString &text);
    QString speechRecognitionLanguage() const;
    QString normalizeSpeechRecognitionLanguage(const QString &language) const;
    QString speechContextId() const;
    bool    canInsertImages() const;
    bool    insertImportedImage(const MediaReference &reference);

    Ui::NoteWidget *ui        = nullptr;
    QmlNoteEditor  *qmlEditor = nullptr;

    QAction                              *mdModeAct;
    QAction                              *txtModeAct;
    QAction                              *speechAction      = nullptr;
    QAction                              *pinAction         = nullptr;
    QAction                              *insertImageAction = nullptr;
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
    QUuid                                 _draftId;
    quint64                               _draftRevision = 0;
    QString                               _baselineText;
    Note::Format                          _baselineFormat = Note::PlainText;
    QString                               _firstLine;
    QString                               _extFileName;
    QString                               _extSelecteFilter;
    QTimer                                _autosaveTimer;
    QTimer                                _mediaResizeTimer;
    QElapsedTimer                         _lastChangeElapsed;
    Features                              _features;
    bool                                  _trashRequested       = false;
    bool                                  _changed              = false;
    bool                                  _draftPersisted       = false;
    bool                                  _sessionReleased      = false;
    bool                                  speechRecognitionBusy = false;
};

} // namespace QtNote

Q_DECLARE_OPERATORS_FOR_FLAGS(QtNote::NoteWidget::Features)

#endif // NOTEWIDGET_H
