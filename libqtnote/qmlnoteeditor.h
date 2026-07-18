#ifndef QMLNOTEEDITOR_H
#define QMLNOTEEDITOR_H

#include <QPointer>
#include <QWidget>
#include <memory>

#include "mediareference.h"
#include "note.h"

class QQuickWidget;
class QQuickTextDocument;
class QImage;

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
    Q_INVOKABLE void         registerTextDocument(QQuickTextDocument *document, bool titleDocument);
    Q_INVOKABLE QVariantList spellCheckRanges(QQuickTextDocument *document);
    Q_INVOKABLE QStringList  spellingSuggestions(const QString &word) const;
    Q_INVOKABLE void         addToSpellingDictionary(const QString &word);
    Q_INVOKABLE void         copyToClipboard(const QString &text);
    bool                     spellCheckEnabled() const { return spellCheckEnabled_; }
    void                     setSpellCheckEnabled(bool enabled);
    void                     addHighlightExtension(const std::shared_ptr<HighlighterExtension> &extension, int type);
    void                     rehighlight();

signals:
    void contentsChanged();
    void focusReceived();
    void focusLost();
    void imagePasteRequested(const QImage &image);
    void spellCheckEnabledChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
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
    bool                         spellCheckEnabled_ = true;
};
} // namespace QtNote

#endif
