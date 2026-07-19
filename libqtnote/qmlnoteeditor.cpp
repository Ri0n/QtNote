#include "qmlnoteeditor.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMimeData>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QQuickWidget>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextLayout>
#include <QTimer>
#include <QVBoxLayout>

#include "localmediastore.h"
#include "noteblockmodel.h"
#include "notehighlighter.h"

namespace QtNote {
namespace {
    class LocalMediaImageProvider : public QQuickImageProvider {
    public:
        LocalMediaImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) { }

        QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
        {
            const auto loaded = LocalMediaStore::instance()->data(QByteArray::fromHex(id.toLatin1()));
            QImage     image;
            if (loaded)
                image.loadFromData(loaded.value);
            if (size)
                *size = image.size();
            if (!image.isNull() && requestedSize.isValid())
                image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            return image;
        }
    };
}

QmlNoteEditor::QmlNoteEditor(QWidget *parent) : QWidget(parent), model_(new NoteBlockModel(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    quick_ = new QQuickWidget(this);
    setFocusPolicy(Qt::StrongFocus);
    quick_->setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(quick_);
    quick_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_->setClearColor(palette().color(QPalette::Base));
    quick_->engine()->addImageProvider(QStringLiteral("qtnote-media"), new LocalMediaImageProvider);
    quick_->rootContext()->setContextProperty(QStringLiteral("noteBlockModel"), model_);
    quick_->rootContext()->setContextProperty(QStringLiteral("qmlNoteEditor"), this);
    quick_->setSource(QUrl(QStringLiteral("qrc:/qml/NoteBlockEditor.qml")));
    quick_->installEventFilter(this);
    layout->addWidget(quick_);

    connect(model_, &NoteBlockModel::contentsChanged, this, &QmlNoteEditor::contentsChanged);
}

void QmlNoteEditor::load(const QString &contents, Note::Format format)
{
    model_->load(contents, format != Note::PlainText);
}

void QmlNoteEditor::setMedia(const QList<MediaReference> &media)
{
    QHash<QString, QString> urls;
    for (const auto &reference : media) {
        if (reference.isValid() && reference.mediaType.startsWith(QLatin1String("image/"))) {
            urls.insert(reference.uri(),
                        QStringLiteral("image://qtnote-media/%1").arg(QString::fromLatin1(reference.blobId.toHex())));
        }
    }
    model_->setPreviewUrls(urls);
}

QString QmlNoteEditor::contents() const { return model_->contents(); }
bool    QmlNoteEditor::isMarkdown() const { return model_->markdown(); }

void QmlNoteEditor::insertText(const QString &text)
{
    QVariant inserted;
    if (quick_->rootObject()
        && QMetaObject::invokeMethod(quick_->rootObject(), "insertTextAtCursor", Q_RETURN_ARG(QVariant, inserted),
                                     Q_ARG(QVariant, text))
        && inserted.toBool()) {
        return;
    }
    model_->appendText(text);
}

void QmlNoteEditor::focusEditor()
{
    quick_->setFocus(Qt::OtherFocusReason);
    if (quick_->rootObject())
        QMetaObject::invokeMethod(quick_->rootObject(), "focusInitialEditor");
}

void QmlNoteEditor::insertTable()
{
    if (quick_->rootObject())
        QMetaObject::invokeMethod(quick_->rootObject(), "insertTableBlock");
}

void QmlNoteEditor::insertList(int type)
{
    if (quick_->rootObject())
        QMetaObject::invokeMethod(quick_->rootObject(), "insertListBlock", Q_ARG(QVariant, type));
}

int QmlNoteEditor::applyInlineFormat(QQuickTextDocument *quickDocument, int start, int end, const QString &style)
{
    if (!quickDocument || !quickDocument->textDocument())
        return -1;
    auto *document = quickDocument->textDocument();
    start          = qBound(0, start, document->characterCount() - 1);
    end            = qBound(start, end, document->characterCount() - 1);
    QTextCursor cursor(document);
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);

    QString placeholder;
    if (!cursor.hasSelection()) {
        placeholder = style == QLatin1String("code") ? tr("code")
            : style == QLatin1String("link")         ? tr("link")
                                                     : tr("text");
        cursor.insertText(placeholder);
        cursor.setPosition(start);
        cursor.setPosition(start + placeholder.size(), QTextCursor::KeepAnchor);
        end = start + placeholder.size();
    }

    const auto      current = cursor.charFormat();
    QTextCharFormat format;
    if (style == QLatin1String("bold"))
        format.setFontWeight(current.fontWeight() >= QFont::Bold ? QFont::Normal : QFont::Bold);
    else if (style == QLatin1String("italic"))
        format.setFontItalic(!current.fontItalic());
    else if (style == QLatin1String("strike"))
        format.setFontStrikeOut(!current.fontStrikeOut());
    else if (style == QLatin1String("code")) {
        format.setFontFixedPitch(!current.fontFixedPitch());
        if (!current.fontFixedPitch())
            format.setFontFamilies({ QStringLiteral("monospace") });
    } else if (style == QLatin1String("link")) {
        format.setAnchor(!current.isAnchor());
        format.setAnchorHref(current.isAnchor() ? QString() : QStringLiteral("url"));
    } else {
        return -1;
    }
    cursor.mergeCharFormat(format);
    return end;
}

void QmlNoteEditor::registerTextDocument(QQuickTextDocument *document, bool titleDocument)
{
    if (!document || !document->textDocument())
        return;
    auto highlighter = new NoteHighlighter(document->textDocument());
    for (const auto &item : extensions_) {
        const auto type = NoteHighlighter::ExtType(item.type);
        if (type == NoteHighlighter::Other || (type == NoteHighlighter::Title && !titleDocument))
            continue;
        highlighter->addExtension(item.extension, type);
        if (type == NoteHighlighter::SpellCheck && !spellCheckEnabled_)
            highlighter->disableExtension(type);
    }
    highlighters_.append({ highlighter, titleDocument });
    qInfo() << "QML text document registered" << static_cast<const void *>(document->textDocument())
            << "title=" << titleDocument << "extensions=" << extensions_.size();
    highlighter->rehighlight();
}

QVariantList QmlNoteEditor::spellCheckRanges(QQuickTextDocument *document)
{
    QVariantList result;
    if (!spellCheckEnabled_ || !document || !document->textDocument())
        return result;
    for (auto block = document->textDocument()->begin(); block.isValid(); block = block.next()) {
        if (!block.layout())
            continue;
        for (const auto &range : block.layout()->formats()) {
            if (!range.format.property(SpellCheckFormatProperty).toBool())
                continue;
            QVariantMap value;
            value.insert(QStringLiteral("start"), block.position() + range.start);
            value.insert(QStringLiteral("length"), range.length);
            result.append(value);
        }
    }
    return result;
}

QStringList QmlNoteEditor::spellingSuggestions(const QString &word) const
{
    if (!spellCheckEnabled_)
        return {};
    for (const auto &item : extensions_) {
        if (item.type != int(NoteHighlighter::SpellCheck))
            continue;
        if (auto spell = std::dynamic_pointer_cast<SpellCheckExtension>(item.extension))
            return spell->suggestions(word);
    }
    return {};
}

void QmlNoteEditor::copyToClipboard(const QString &text) { QGuiApplication::clipboard()->setText(text); }

void QmlNoteEditor::setSpellCheckEnabled(bool enabled)
{
    if (spellCheckEnabled_ == enabled)
        return;
    spellCheckEnabled_ = enabled;
    for (const auto &registered : std::as_const(highlighters_)) {
        if (!registered.highlighter)
            continue;
        if (enabled)
            registered.highlighter->enableExtension(NoteHighlighter::SpellCheck);
        else
            registered.highlighter->disableExtension(NoteHighlighter::SpellCheck);
    }
    rehighlight();
    emit spellCheckEnabledChanged();
}

void QmlNoteEditor::addToSpellingDictionary(const QString &word)
{
    for (const auto &item : extensions_) {
        if (item.type != int(NoteHighlighter::SpellCheck))
            continue;
        if (auto spell = std::dynamic_pointer_cast<SpellCheckExtension>(item.extension)) {
            spell->addToDictionary(word);
            rehighlight();
            return;
        }
    }
}

void QmlNoteEditor::addHighlightExtension(const std::shared_ptr<HighlighterExtension> &extension, int type)
{
    extensions_.append({ extension, type });
    int attached = 0;
    for (const auto &registered : std::as_const(highlighters_)) {
        if (registered.highlighter && type != int(NoteHighlighter::Other)
            && (type != int(NoteHighlighter::Title) || registered.titleDocument)) {
            registered.highlighter->addExtension(extension, NoteHighlighter::ExtType(type));
            ++attached;
        }
    }
    qInfo() << "QML highlight extension added: type=" << type << "documents=" << highlighters_.size()
            << "attached=" << attached;
    rehighlight();
}

void QmlNoteEditor::rehighlight()
{
    highlighters_.removeIf([](const auto &registered) { return registered.highlighter.isNull(); });
    for (const auto &registered : std::as_const(highlighters_))
        registered.highlighter->rehighlight();
}

bool QmlNoteEditor::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == quick_) {
        if (event->type() == QEvent::KeyPress) {
            auto keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->matches(QKeySequence::Paste)) {
                const auto mime = QGuiApplication::clipboard()->mimeData();
                if (mime && mime->hasImage()) {
                    const auto image = qvariant_cast<QImage>(mime->imageData());
                    if (!image.isNull()) {
                        emit imagePasteRequested(image);
                        return true;
                    }
                }
            }
        }
        if (event->type() == QEvent::FocusIn) {
            emit focusReceived();
            QTimer::singleShot(0, this, &QmlNoteEditor::focusEditor);
        } else if (event->type() == QEvent::FocusOut)
            emit focusLost();
    }
    return QWidget::eventFilter(watched, event);
}
} // namespace QtNote
