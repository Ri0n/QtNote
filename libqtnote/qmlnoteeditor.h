#ifndef QMLNOTEEDITOR_H
#define QMLNOTEEDITOR_H

#include <QPointer>
#include <QStringList>
#include <QVariantMap>
#include <QWidget>
#include <memory>

#include "mediareference.h"
#include "note.h"
#include "notefragment.h"

class QQuickWidget;
class QQuickTextDocument;
class QImage;
class QMimeData;
class QPointF;
class QShowEvent;
class QTemporaryDir;

namespace QtNote {
class NoteBlockModel;
class NoteDocumentHistory;
class NoteHighlighter;
class HighlighterExtension;

class QTNOTE_EXPORT QmlNoteEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool spellCheckEnabled READ spellCheckEnabled WRITE setSpellCheckEnabled NOTIFY spellCheckEnabledChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoStateChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoStateChanged)
    Q_PROPERTY(QString undoText READ undoText NOTIFY undoStateChanged)
    Q_PROPERTY(QString redoText READ redoText NOTIFY undoStateChanged)
    Q_PROPERTY(bool markdown READ isMarkdown NOTIFY formatChanged)

public:
    enum class LoadPolicy {
        ResetHistory,
        RecordFormatConversion,
        HistoryRestore,
    };
    Q_ENUM(LoadPolicy)

    explicit QmlNoteEditor(QWidget *parent = nullptr);
    ~QmlNoteEditor() override;

    NoteBlockModel  *model() const { return model_; }
    void             load(const QString &contents, Note::Format format, LoadPolicy policy = LoadPolicy::ResetHistory);
    void             setMedia(const QList<MediaReference> &media);
    void             setImageInsertionEnabled(bool enabled) { imageInsertionEnabled_ = enabled; }
    QString          contents() const;
    bool             isMarkdown() const;
    bool             canUndo() const;
    bool             canRedo() const;
    QString          undoText() const;
    QString          redoText() const;
    void             insertText(const QString &text);
    void             focusEditor();
    void             insertTable();
    void             insertList(int type);
    void             beginExternalHistoryTransaction(const QString &kind);
    void             endExternalHistoryTransaction();
    void             breakHistoryMerge();
    Q_INVOKABLE void registerTextDocument(QQuickTextDocument *document, bool titleDocument);
    Q_INVOKABLE QVariantList spellCheckRanges(QQuickTextDocument *document);
    Q_INVOKABLE QStringList  spellingSuggestions(const QString &word) const;
    Q_INVOKABLE void         addToSpellingDictionary(const QString &word);
    Q_INVOKABLE void         copyToClipboard(const QString &text);
    Q_INVOKABLE void         copyMarkdownToClipboard(const QString &markdown);
    Q_INVOKABLE void         saveImageAs(const QString &url);
    Q_INVOKABLE bool         startImageDrag(int row);
    Q_INVOKABLE void         copyDocumentToClipboard();
    Q_INVOKABLE bool         copySelectionToClipboard(const QVariantList &ranges);
    Q_INVOKABLE QVariantMap  deleteSelection(const QVariantList &ranges);
    Q_INVOKABLE QVariantMap  pasteStructuredFromClipboard(QQuickTextDocument *document, int row, int start, int end);
    Q_INVOKABLE QVariantMap  pasteTableFromClipboard(int row, int cell);
    Q_INVOKABLE QVariantMap pasteListFromClipboard(QQuickTextDocument *document, int row, int item, int start, int end);
    Q_INVOKABLE QVariantMap linkInfo(QQuickTextDocument *document, int start, int end) const;
    Q_INVOKABLE int         setLink(QQuickTextDocument *document, int start, int end, const QString &href);
    Q_INVOKABLE bool        primaryModifierPressed() const;
    Q_INVOKABLE int         applyInlineFormat(QQuickTextDocument *document, int start, int end, const QString &style);
    Q_INVOKABLE QString     markdownText(QQuickTextDocument *document) const;
    Q_INVOKABLE QString     markdownSelection(QQuickTextDocument *document, int start, int end) const;
    Q_INVOKABLE void        beginHistoryTransaction(const QString &kind, const QVariantMap &beforeView);
    Q_INVOKABLE void        endHistoryTransaction(const QVariantMap &afterView);
    Q_INVOKABLE void        updateHistoryViewState(const QVariantMap &viewState, bool breakMerge = false);
    Q_INVOKABLE bool        undo();
    Q_INVOKABLE bool        redo();
    bool                    spellCheckEnabled() const { return spellCheckEnabled_; }
    void                    setSpellCheckEnabled(bool enabled);
    void                    addHighlightExtension(const std::shared_ptr<HighlighterExtension> &extension, int type);
    void                    rehighlight();

signals:
    void contentsChanged();
    void focusReceived();
    void focusLost();
    void imagePasteRequested(const QImage &image);
    void imageDropRequested(const QImage &image, int row);
    void imageFilesDropRequested(const QStringList &fileNames, int row);
    void mediaInserted(const QList<MediaReference> &references);
    void mediaChanged(const QList<MediaReference> &references);
    void spellCheckEnabledChanged();
    void undoStateChanged();
    void formatChanged(bool markdown);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void            updateFocusWindow();
    void            flushPendingEditorChanges();
    void            prepareForHistoryRestore();
    void            scheduleHistoryViewRestore(const QVariantMap &viewState);
    void            restoreScalarField(int blockIndex, int role, int fieldIndex, const QString &value);
    NoteFragment    documentFragment() const;
    NoteFragment    withMedia(NoteFragment fragment) const;
    bool            canAcceptImageDrop(const QMimeData *mimeData) const;
    bool            handleImageDrop(const QMimeData *mimeData, int row);
    int             insertionRowAt(const QPointF &position) const;
    QString         materializeDragImage(const MediaReference &reference, const QByteArray &data);
    NoteBlockModel *model_ = nullptr;
    QQuickWidget   *quick_ = nullptr;
    struct HighlightExtension {
        std::shared_ptr<HighlighterExtension> extension;
        int                                   type;
    };
    struct RegisteredHighlighter {
        QPointer<NoteHighlighter> highlighter;
        bool                      titleDocument;
    };
    QList<HighlightExtension>            extensions_;
    QList<RegisteredHighlighter>         highlighters_;
    QPointer<QWidget>                    focusWindow_;
    QString                              baselineOverrideContents_;
    quint64                              loadGeneration_             = 0;
    bool                                 baselineOverrideMarkdown_   = false;
    bool                                 baselineOverrideActive_     = false;
    bool                                 suppressNextFocusRefresh_   = false;
    bool                                 hasLoaded_                  = false;
    bool                                 spellCheckEnabled_          = true;
    bool                                 scalarHistoryChangePending_ = false;
    bool                                 imageInsertionEnabled_      = false;
    bool                                 imageDragAccepted_          = false;
    QList<MediaReference>                media_;
    std::unique_ptr<NoteDocumentHistory> history_;
    std::unique_ptr<QTemporaryDir>       dragExportDirectory_;
};
} // namespace QtNote

#endif
