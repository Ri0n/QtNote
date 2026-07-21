#ifndef QMLNOTEEDITOR_H
#define QMLNOTEEDITOR_H

#include <QPointer>
#include <QVariantMap>
#include <QWidget>
#include <memory>

#include "mediareference.h"
#include "note.h"
#include "notefragment.h"

class QQuickWidget;
class QQuickTextDocument;
class QImage;
class QShowEvent;

namespace QtNote {
class NoteBlockModel;
class NoteHighlighter;
class HighlighterExtension;

class QTNOTE_EXPORT QmlNoteEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool spellCheckEnabled READ spellCheckEnabled WRITE setSpellCheckEnabled NOTIFY spellCheckEnabledChanged)

public:
    explicit QmlNoteEditor(QWidget *parent = nullptr);

    NoteBlockModel          *model() const { return model_; }
    void                     load(const QString &contents, Note::Format format);
    void                     setMedia(const QList<MediaReference> &media);
    QString                  contents() const;
    bool                     isMarkdown() const;
    void                     insertText(const QString &text);
    void                     focusEditor();
    void                     insertTable();
    void                     insertList(int type);
    Q_INVOKABLE void         registerTextDocument(QQuickTextDocument *document, bool titleDocument);
    Q_INVOKABLE QVariantList spellCheckRanges(QQuickTextDocument *document);
    Q_INVOKABLE QStringList  spellingSuggestions(const QString &word) const;
    Q_INVOKABLE void         addToSpellingDictionary(const QString &word);
    Q_INVOKABLE void         copyToClipboard(const QString &text);
    Q_INVOKABLE void         copyMarkdownToClipboard(const QString &markdown);
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
    bool                    spellCheckEnabled() const { return spellCheckEnabled_; }
    void                    setSpellCheckEnabled(bool enabled);
    void                    addHighlightExtension(const std::shared_ptr<HighlighterExtension> &extension, int type);
    void                    rehighlight();

signals:
    void contentsChanged();
    void focusReceived();
    void focusLost();
    void imagePasteRequested(const QImage &image);
    void mediaInserted(const QList<MediaReference> &references);
    void spellCheckEnabledChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void            updateFocusWindow();
    NoteFragment    documentFragment() const;
    NoteFragment    withMedia(NoteFragment fragment) const;
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
    QList<HighlightExtension>    extensions_;
    QList<RegisteredHighlighter> highlighters_;
    QPointer<QWidget>            focusWindow_;
    QString                      baselineOverrideContents_;
    quint64                      loadGeneration_           = 0;
    bool                         baselineOverrideMarkdown_ = false;
    bool                         baselineOverrideActive_   = false;
    bool                         suppressNextFocusRefresh_ = false;
    bool                         hasLoaded_                = false;
    bool                         spellCheckEnabled_        = true;
    QList<MediaReference>        media_;
};
} // namespace QtNote

#endif
