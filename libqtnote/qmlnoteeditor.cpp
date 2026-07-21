#ifdef QTNOTE_EDITOR_CORE
#include "noteeditor.h"
#else
#include "qmlnoteeditor.h"
#endif

#include <QClipboard>
#ifndef QTNOTE_EDITOR_CORE
#include <QDir>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#endif
#include <QFont>
#include <QGuiApplication>
#ifndef QTNOTE_EDITOR_CORE
#include <QKeyEvent>
#include <QKeySequence>
#include <QMessageBox>
#endif
#include <QMimeData>
#include <QPalette>
#ifndef QTNOTE_EDITOR_CORE
#include <QMimeDatabase>
#include <QPixmap>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QQuickItem>
#endif
#include <QQuickTextDocument>
#include <QRegularExpression>
#ifndef QTNOTE_EDITOR_CORE
#include <QQuickWidget>
#include <QSaveFile>
#include <QStandardPaths>
#endif
#include <QStringList>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFragment>
#include <QTextLayout>
#ifndef QTNOTE_EDITOR_CORE
#include <QTimer>
#include <QVBoxLayout>
#endif
#include <QVector>
#include <algorithm>

#include "localmediastore.h"
#include "noteblockmodel.h"
#include "notefragmentmediatransfer.h"
#ifndef QTNOTE_EDITOR_CORE
#include "noteeditor.h"
#include "notehighlighter.h"
#endif
#include "notetransfercontroller.h"

namespace QtNote {
namespace {
#ifndef QTNOTE_EDITOR_CORE
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
#endif

    int documentEnd(const QTextDocument *document) { return document ? qMax(0, document->characterCount() - 1) : 0; }

#ifndef QTNOTE_EDITOR_CORE
    bool isUsableImageFragment(const NoteFragment &fragment)
    {
        if (fragment.blocks.isEmpty())
            return false;
        for (const NoteFragmentBlock &block : fragment.blocks) {
            if (block.type != NoteFragmentBlockType::Image || block.image.sourceUri.isEmpty())
                return false;
            const QUrl source(block.image.sourceUri);
            if (source.scheme().compare(QStringLiteral("qtnote-media"), Qt::CaseInsensitive) != 0)
                continue;
            const bool hasMedia = std::any_of(fragment.media.cbegin(), fragment.media.cend(), [&block](const auto &m) {
                return m.sourceUri == block.image.sourceUri && m.reference.isValid();
            });
            if (!hasMedia)
                return false;
        }
        return true;
    }

    QStringList localImageFiles(const QMimeData *mimeData)
    {
        QStringList result;
        if (!mimeData || !mimeData->hasUrls())
            return result;
        QMimeDatabase database;
        for (const QUrl &url : mimeData->urls()) {
            const QString fileName = url.toLocalFile();
            if (fileName.isEmpty() || !QFileInfo(fileName).isFile())
                continue;
            const QMimeType type = database.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);
            if (type.name().startsWith(QLatin1String("image/")))
                result.append(fileName);
        }
        return result;
    }
#endif

    QString unwrapMarkdownWriterLines(const QString &markdown);
    QString markdownWithSerializedInlineFormats(QTextDocument *document);

    QString markdownRange(QTextDocument *document, int start, int end)
    {
        if (!document)
            return {};
        const int limit = documentEnd(document);
        start           = qBound(0, start, limit);
        end             = qBound(start, end, limit);
        if (start == end)
            return {};
        QTextCursor cursor(document);
        cursor.setPosition(start);
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        QTextDocument selection;
        QTextCursor   destination(&selection);
        destination.insertFragment(QTextDocumentFragment(cursor));
        QString markdown = unwrapMarkdownWriterLines(markdownWithSerializedInlineFormats(&selection));
        while (markdown.endsWith(QLatin1Char('\n')) || markdown.endsWith(QLatin1Char('\r')))
            markdown.chop(1);
        return markdown;
    }

    QString unwrapMarkdownWriterLines(const QString &markdown)
    {
        const QStringList lines = markdown.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
        QString           result;
        bool              previousWasContent = false;
        for (const QString &line : lines) {
            if (line.isEmpty()) {
                if (!result.endsWith(QStringLiteral("\n\n"))) {
                    if (result.endsWith(QLatin1Char('\n')))
                        result += QLatin1Char('\n');
                    else
                        result += QStringLiteral("\n\n");
                }
                previousWasContent = false;
                continue;
            }
            if (previousWasContent) {
                // Two trailing spaces are an explicit Markdown hard break.
                // Every other newline inside one QTextDocument block is a
                // soft wrap introduced by Qt's Markdown writer.
                result += result.endsWith(QStringLiteral("  ")) ? QLatin1Char('\n') : QLatin1Char(' ');
                result += line.trimmed();
            } else {
                result += line;
            }
            previousWasContent = true;
        }
        return result;
    }

#ifndef QTNOTE_EDITOR_CORE
    bool invokeQmlBoolean(QObject *object, const char *method)
    {
        if (!object)
            return false;
        QVariant result;
        return QMetaObject::invokeMethod(object, method, Q_RETURN_ARG(QVariant, result)) && result.toBool();
    }

    QVariantMap captureQmlEditorState(QObject *object)
    {
        if (!object)
            return {};
        QVariant result;
        if (!QMetaObject::invokeMethod(object, "captureEditorState", Q_RETURN_ARG(QVariant, result)))
            return {};
        return result.toMap();
    }
#endif

    QTextCharFormat formatAt(QTextDocument *document, int position)
    {
        const int limit = documentEnd(document);
        if (!document || position < 0 || position >= limit)
            return {};

        const QTextBlock block = document->findBlock(position);
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (fragment.isValid() && fragment.contains(position))
                return fragment.charFormat();
        }
        return {};
    }

    void setLinkOnRange(QTextDocument *document, int start, int end, const QString &href)
    {
        if (!document || start >= end)
            return;

        struct FormatRange {
            int             start;
            int             end;
            QTextCharFormat format;
        };
        QVector<FormatRange> ranges;

        for (QTextBlock block = document->findBlock(start); block.isValid() && block.position() < end;
             block            = block.next()) {
            for (auto it = block.begin(); !it.atEnd(); ++it) {
                const QTextFragment fragment = it.fragment();
                if (!fragment.isValid())
                    continue;
                const int rangeStart = qMax(start, fragment.position());
                const int rangeEnd   = qMin(end, fragment.position() + fragment.length());
                if (rangeStart >= rangeEnd)
                    continue;

                QTextCharFormat format = fragment.charFormat();
                if (href.isEmpty()) {
                    format.setAnchor(false);
                    format.clearProperty(QTextFormat::AnchorHref);
                    format.clearProperty(QTextFormat::AnchorName);
                    format.clearProperty(QTextFormat::ForegroundBrush);
                } else {
                    format.setAnchor(true);
                    format.setAnchorHref(href);
                    format.setForeground(QGuiApplication::palette().link());
                }
                ranges.append({ rangeStart, rangeEnd, format });
            }
        }

        for (const auto &range : ranges) {
            QTextCursor cursor(document);
            cursor.setPosition(range.start);
            cursor.setPosition(range.end, QTextCursor::KeepAnchor);
            cursor.setCharFormat(range.format);
        }
    }

    const QString LinkLabelMarkerPrefix = QStringLiteral("QTNOTELINKLABEL7F3A");
    const QString LinkLabelMarkerSuffix = QStringLiteral("END7F3A");

    QString protectLinkLabels(const QString &markdown)
    {
        if (!markdown.contains(QLatin1Char('[')))
            return markdown;

        // Keep the complete Markdown spelling of a link label opaque while
        // NoteBlockModel canonicalizes the document through QTextDocument.
        // QTextDocument otherwise drops or rearranges nested emphasis located
        // in the middle of a word inside a link label.
        static const QRegularExpression link(
            QStringLiteral(R"((?<!!)\[((?:\\.|[^\]\\\n])*)\]\(((?:\\.|[^)\\\n])*)\))"));

        QString result;
        int     copiedUntil = 0;
        auto    links       = link.globalMatch(markdown);
        while (links.hasNext()) {
            const auto       match   = links.next();
            const QByteArray encoded = match.captured(1).toUtf8().toHex().toUpper();

            result += markdown.mid(copiedUntil, match.capturedStart() - copiedUntil);
            result += QLatin1Char('[') + LinkLabelMarkerPrefix + QString::fromLatin1(encoded) + LinkLabelMarkerSuffix
                + QStringLiteral("](") + match.captured(2) + QLatin1Char(')');
            copiedUntil = match.capturedEnd();
        }

        if (copiedUntil == 0)
            return markdown;
        result += markdown.mid(copiedUntil);
        return result;
    }

    QString restoreLinkLabels(const QString &markdown)
    {
        static const QRegularExpression marker(LinkLabelMarkerPrefix + QStringLiteral("([0-9A-Fa-f]+)")
                                               + LinkLabelMarkerSuffix);

        QString result;
        int     copiedUntil = 0;
        auto    markers     = marker.globalMatch(markdown);
        while (markers.hasNext()) {
            const auto       match   = markers.next();
            const QByteArray decoded = QByteArray::fromHex(match.captured(1).toLatin1());

            result += markdown.mid(copiedUntil, match.capturedStart() - copiedUntil);
            result += QString::fromUtf8(decoded);
            copiedUntil = match.capturedEnd();
        }

        if (copiedUntil == 0)
            return markdown;
        result += markdown.mid(copiedUntil);
        return result;
    }

    void restoreProtectedMarkdown(NoteBlockModel *model)
    {
        if (!model)
            return;

        for (int row = 0; row < model->rowCount(); ++row) {
            const QModelIndex modelIndex = model->index(row);
            const auto type = NoteBlockModel::BlockType(model->data(modelIndex, NoteBlockModel::TypeRole).toInt());

            switch (type) {
            case NoteBlockModel::Text:
            case NoteBlockModel::Heading: {
                const QString text     = model->data(modelIndex, NoteBlockModel::TextRole).toString();
                const QString restored = restoreLinkLabels(text);
                if (restored != text)
                    model->setBlockText(row, restored);
                break;
            }
            case NoteBlockModel::BulletList:
            case NoteBlockModel::CheckList:
            case NoteBlockModel::NumberedList: {
                const QStringList items = model->data(modelIndex, NoteBlockModel::ItemsRole).toStringList();
                for (int item = 0; item < items.size(); ++item) {
                    const QString restored = restoreLinkLabels(items.at(item));
                    if (restored != items.at(item))
                        model->setListItem(row, item, restored);
                }
                break;
            }
            case NoteBlockModel::Table: {
                const QVariantMap table = model->data(modelIndex, NoteBlockModel::CellsRole).toMap();
                const QStringList cells = table.value(QStringLiteral("values")).toStringList();
                for (int cell = 0; cell < cells.size(); ++cell) {
                    const QString restored = restoreLinkLabels(cells.at(cell));
                    if (restored != cells.at(cell))
                        model->setTableCell(row, cell, restored);
                }
                break;
            }
            case NoteBlockModel::Image:
                break;
            }
        }
    }
    struct InlineStyle {
        bool bold      = false;
        bool italic    = false;
        bool strike    = false;
        bool code      = false;
        bool underline = false;

        bool operator==(const InlineStyle &other) const
        {
            return bold == other.bold && italic == other.italic && strike == other.strike && code == other.code
                && underline == other.underline;
        }
    };

    struct LinkGroup {
        int                  start = 0;
        int                  end   = 0;
        QString              href;
        QString              text;
        QVector<InlineStyle> styles;
    };

    struct CodeGroup {
        int         start = 0;
        int         end   = 0;
        QString     text;
        InlineStyle style;
    };

    InlineStyle explicitInlineStyle(const QTextCharFormat &format)
    {
        InlineStyle style;
        style.bold      = format.hasProperty(QTextFormat::FontWeight) && format.fontWeight() >= QFont::Bold;
        style.italic    = format.hasProperty(QTextFormat::FontItalic) && format.fontItalic();
        style.strike    = format.hasProperty(QTextFormat::FontStrikeOut) && format.fontStrikeOut();
        style.code      = format.hasProperty(QTextFormat::FontFixedPitch) && format.fontFixedPitch();
        style.underline = format.hasProperty(QTextFormat::TextUnderlineStyle)
            && format.underlineStyle() != QTextCharFormat::NoUnderline;
        return style;
    }

    QVector<LinkGroup> documentLinkGroups(const QTextDocument *document)
    {
        QVector<LinkGroup> groups;
        if (!document)
            return groups;

        int     previousEnd = -1;
        QString previousHref;
        for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
            for (auto it = block.begin(); !it.atEnd(); ++it) {
                const QTextFragment fragment = it.fragment();
                if (!fragment.isValid() || fragment.length() <= 0)
                    continue;

                const QTextCharFormat format = fragment.charFormat();
                const QString         href   = format.isAnchor() ? format.anchorHref() : QString();
                if (href.isEmpty()) {
                    previousEnd = -1;
                    previousHref.clear();
                    continue;
                }

                const bool continuesGroup
                    = !groups.isEmpty() && previousEnd == fragment.position() && previousHref == href;
                if (!continuesGroup) {
                    LinkGroup group;
                    group.start = fragment.position();
                    group.href  = href;
                    groups.append(group);
                }

                auto             &group        = groups.last();
                const QString     fragmentText = fragment.text();
                const InlineStyle style        = explicitInlineStyle(format);
                group.text += fragmentText;
                for (qsizetype index = 0; index < fragmentText.size(); ++index)
                    group.styles.append(style);

                group.end    = fragment.position() + fragment.length();
                previousEnd  = group.end;
                previousHref = href;
            }
            previousEnd = -1;
            previousHref.clear();
        }
        return groups;
    }

    QVector<CodeGroup> documentCodeGroups(const QTextDocument *document)
    {
        QVector<CodeGroup> groups;
        if (!document)
            return groups;
        for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
            for (auto it = block.begin(); !it.atEnd(); ++it) {
                const QTextFragment fragment = it.fragment();
                if (!fragment.isValid() || fragment.length() <= 0)
                    continue;
                const QTextCharFormat format = fragment.charFormat();
                const InlineStyle     style  = explicitInlineStyle(format);
                if ((format.isAnchor() && !format.anchorHref().isEmpty()) || !style.code)
                    continue;
                if (!groups.isEmpty() && groups.last().end == fragment.position() && groups.last().style == style) {
                    groups.last().end = fragment.position() + fragment.length();
                    groups.last().text += fragment.text();
                } else {
                    groups.append(
                        { fragment.position(), fragment.position() + fragment.length(), fragment.text(), style });
                }
            }
        }
        return groups;
    }

    QString escapeMarkdownLabelText(const QString &text)
    {
        QString escaped;
        escaped.reserve(text.size() * 2);
        static const QString special = QStringLiteral(R"(\\`*[]_~)");
        for (const QChar character : text) {
            if (character == QChar::ParagraphSeparator || character == QLatin1Char('\n')) {
                escaped += QStringLiteral("<br>");
                continue;
            }
            if (special.contains(character))
                escaped += QLatin1Char('\\');
            escaped += character;
        }
        return escaped;
    }

    QString codeSpan(const QString &text)
    {
        int maximumRun = 0;
        int currentRun = 0;
        for (const QChar character : text) {
            if (character == QLatin1Char('`')) {
                maximumRun = qMax(maximumRun, ++currentRun);
            } else {
                currentRun = 0;
            }
        }
        const QString delimiter(maximumRun + 1, QLatin1Char('`'));
        return delimiter + text + delimiter;
    }

    QVector<QString> inlineDelimiters(const InlineStyle &style)
    {
        QVector<QString> delimiters;
        if (style.underline)
            delimiters.append(QStringLiteral("<ins>"));
        if (style.strike)
            delimiters.append(QStringLiteral("~~"));
        if (style.bold)
            delimiters.append(QStringLiteral("**"));
        if (style.italic)
            delimiters.append(QStringLiteral("*"));
        return delimiters;
    }

    QString closingInlineDelimiter(const QString &delimiter)
    {
        return delimiter == QLatin1String("<ins>") ? QStringLiteral("</ins>") : delimiter;
    }

    QString serializedMarkdownCode(CodeGroup group)
    {
        group.style.code                  = false;
        const QVector<QString> delimiters = inlineDelimiters(group.style);
        QString                result;
        for (const QString &delimiter : delimiters)
            result += delimiter;
        result += codeSpan(group.text);
        for (int index = delimiters.size() - 1; index >= 0; --index)
            result += closingInlineDelimiter(delimiters.at(index));
        return result;
    }

    QString formattedLinkRange(const LinkGroup &group, int start, int length)
    {
        const int        end = qMin(start + length, int(group.text.size()));
        QString          result;
        QVector<QString> activeDelimiters;

        for (int position = qMax(0, start); position < end;) {
            const InlineStyle style      = group.styles.value(position);
            int               segmentEnd = position + 1;
            while (segmentEnd < end && group.styles.value(segmentEnd) == style)
                ++segmentEnd;

            const QVector<QString> desiredDelimiters = inlineDelimiters(style);
            int                    common            = 0;
            while (common < activeDelimiters.size() && common < desiredDelimiters.size()
                   && activeDelimiters.at(common) == desiredDelimiters.at(common)) {
                ++common;
            }
            for (int index = activeDelimiters.size() - 1; index >= common; --index)
                result += closingInlineDelimiter(activeDelimiters.at(index));
            for (int index = common; index < desiredDelimiters.size(); ++index)
                result += desiredDelimiters.at(index);

            const QString segment = group.text.mid(position, segmentEnd - position);
            result += style.code ? codeSpan(segment) : escapeMarkdownLabelText(segment);
            activeDelimiters = desiredDelimiters;
            position         = segmentEnd;
        }

        for (int index = activeDelimiters.size() - 1; index >= 0; --index)
            result += closingInlineDelimiter(activeDelimiters.at(index));
        return result;
    }

    QString markdownLinkDestination(QString href)
    {
        href.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
        href.replace(QLatin1Char('('), QStringLiteral("\\("));
        href.replace(QLatin1Char(')'), QStringLiteral("\\)"));
        return href;
    }

    QString serializedMarkdownLink(const LinkGroup &group)
    {
        return QLatin1Char('[') + formattedLinkRange(group, 0, group.text.size()) + QStringLiteral("](")
            + markdownLinkDestination(group.href) + QLatin1Char(')');
    }

    struct UnderlineRange {
        int start = 0;
        int end   = 0;
    };

    QVector<UnderlineRange> documentUnderlineRanges(const QTextDocument *document)
    {
        QVector<UnderlineRange> ranges;
        if (!document)
            return ranges;
        for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
            for (auto it = block.begin(); !it.atEnd(); ++it) {
                const QTextFragment fragment = it.fragment();
                if (!fragment.isValid() || fragment.length() <= 0)
                    continue;
                const QTextCharFormat format = fragment.charFormat();
                if ((format.isAnchor() && !format.anchorHref().isEmpty()) || explicitInlineStyle(format).code
                    || !format.fontUnderline())
                    continue;
                if (!ranges.isEmpty() && ranges.last().end == fragment.position())
                    ranges.last().end += fragment.length();
                else
                    ranges.append({ fragment.position(), fragment.position() + fragment.length() });
            }
        }
        return ranges;
    }

    QString markdownWithSerializedInlineFormats(QTextDocument *document)
    {
        const QVector<LinkGroup>      links        = documentLinkGroups(document);
        const QVector<CodeGroup>      codes        = documentCodeGroups(document);
        const QVector<UnderlineRange> underlines   = documentUnderlineRanges(document);
        bool                          hasUnderline = !underlines.isEmpty();
        for (const LinkGroup &group : links)
            hasUnderline = hasUnderline
                || std::any_of(group.styles.cbegin(), group.styles.cend(),
                               [](const InlineStyle &style) { return style.underline; });
        for (const CodeGroup &group : codes)
            hasUnderline = hasUnderline || group.style.underline;
        if (links.isEmpty() && codes.isEmpty() && !hasUnderline)
            return document->toMarkdown(QTextDocument::MarkdownDialectGitHub);

        std::unique_ptr<QTextDocument> copy(document->clone());
        if (hasUnderline) {
            QTextCursor all(copy.get());
            all.select(QTextCursor::Document);
            QTextCharFormat clearUnderline;
            clearUnderline.setFontUnderline(false);
            all.mergeCharFormat(clearUnderline);
        }

        QVector<QString> linkMarkers;
        linkMarkers.reserve(links.size());
        for (int index = 0; index < links.size(); ++index) {
            linkMarkers.append(QStringLiteral("QTNOTESERIALIZEDLINK7F3A%1END7F3A").arg(index));
        }
        QVector<QString> codeMarkers;
        codeMarkers.reserve(codes.size());
        for (int index = 0; index < codes.size(); ++index)
            codeMarkers.append(QStringLiteral("QTNOTESERIALIZEDCODE8C53%1END8C53").arg(index));
        QVector<QPair<QString, QString>> underlineMarkers;
        underlineMarkers.reserve(underlines.size());
        for (int index = 0; index < underlines.size(); ++index) {
            underlineMarkers.append({ QStringLiteral("QTNOTEINSOPEN8C53%1END8C53").arg(index),
                                      QStringLiteral("QTNOTEINSCLOSE8C53%1END8C53").arg(index) });
        }

        struct Replacement {
            enum Kind { Underline, Link, Code };
            int  start = 0;
            int  end   = 0;
            int  index = 0;
            Kind kind  = Underline;
        };
        QVector<Replacement> replacements;
        replacements.reserve(links.size() + codes.size() + underlines.size());
        for (int index = 0; index < links.size(); ++index)
            replacements.append({ links.at(index).start, links.at(index).end, index, Replacement::Link });
        for (int index = 0; index < codes.size(); ++index)
            replacements.append({ codes.at(index).start, codes.at(index).end, index, Replacement::Code });
        for (int index = 0; index < underlines.size(); ++index)
            replacements.append(
                { underlines.at(index).start, underlines.at(index).end, index, Replacement::Underline });
        std::sort(replacements.begin(), replacements.end(),
                  [](const Replacement &left, const Replacement &right) { return left.start > right.start; });

        for (const Replacement &replacement : replacements) {
            if (replacement.kind == Replacement::Link || replacement.kind == Replacement::Code) {
                QTextCursor cursor(copy.get());
                cursor.setPosition(replacement.start);
                cursor.setPosition(replacement.end, QTextCursor::KeepAnchor);
                const QString &marker = replacement.kind == Replacement::Link ? linkMarkers.at(replacement.index)
                                                                              : codeMarkers.at(replacement.index);
                cursor.insertText(marker, QTextCharFormat());
                continue;
            }
            QTextCursor close(copy.get());
            close.setPosition(replacement.end);
            close.insertText(underlineMarkers.at(replacement.index).second, QTextCharFormat());
            QTextCursor open(copy.get());
            open.setPosition(replacement.start);
            open.insertText(underlineMarkers.at(replacement.index).first, QTextCharFormat());
        }

        QString markdown = copy->toMarkdown(QTextDocument::MarkdownDialectGitHub);
        for (int index = 0; index < links.size(); ++index)
            markdown.replace(linkMarkers.at(index), serializedMarkdownLink(links.at(index)));
        for (int index = 0; index < codes.size(); ++index)
            markdown.replace(codeMarkers.at(index), serializedMarkdownCode(codes.at(index)));
        for (int index = 0; index < underlines.size(); ++index) {
            markdown.replace(underlineMarkers.at(index).first, QStringLiteral("<ins>"));
            markdown.replace(underlineMarkers.at(index).second, QStringLiteral("</ins>"));
        }
        return markdown;
    }

    QList<NoteBlockSelectionRange> decodeSelectionRanges(const QVariantList &encodedRanges)
    {
        QList<NoteBlockSelectionRange> ranges;
        ranges.reserve(encodedRanges.size());
        for (const QVariant &encoded : encodedRanges) {
            const QVariantMap       map = encoded.toMap();
            NoteBlockSelectionRange range;
            range.blockIndex     = map.value(QStringLiteral("blockIndex"), -1).toInt();
            range.listItemIndex  = map.value(QStringLiteral("listItemIndex"), -1).toInt();
            range.tableCellIndex = map.value(QStringLiteral("tableCellIndex"), -1).toInt();
            range.markdown       = map.value(QStringLiteral("markdown")).toString();
            range.wholeEditor    = map.value(QStringLiteral("wholeEditor")).toBool();
            range.before         = map.value(QStringLiteral("before")).toString();
            range.after          = map.value(QStringLiteral("after")).toString();
            ranges.append(range);
        }
        return ranges;
    }

}

#ifndef QTNOTE_EDITOR_CORE
QmlNoteEditor::QmlNoteEditor(QWidget *parent) : QmlNoteEditor(nullptr, parent) { }

QmlNoteEditor::QmlNoteEditor(NoteEditor *editor, QWidget *parent) : QWidget(parent), editor_(editor)
{
    if (!editor_)
        editor_ = new NoteEditor({}, {}, this);
    model_ = editor_->model();

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    quick_ = new QQuickWidget(this);
    setFocusPolicy(Qt::StrongFocus);
    quick_->setFocusPolicy(Qt::StrongFocus);
    quick_->setAcceptDrops(true);
    setFocusProxy(quick_);
    quick_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_->setClearColor(palette().color(QPalette::Base));
    quick_->engine()->addImageProvider(QStringLiteral("qtnote-media"), new LocalMediaImageProvider);
    quick_->rootContext()->setContextProperty(QStringLiteral("noteBlockModel"), model_);
    quick_->rootContext()->setContextProperty(QStringLiteral("noteEditor"), editor_);
    quick_->rootContext()->setContextProperty(QStringLiteral("qmlNoteEditor"), this);
    quick_->setSource(QUrl(QStringLiteral("qrc:/qml/NoteBlockEditor.qml")));
    quick_->installEventFilter(this);
    layout->addWidget(quick_);

    editor_->registerEditorView(quick_->rootObject());
    connect(editor_, &NoteEditor::undoStateChanged, this, &QmlNoteEditor::undoStateChanged);
    connect(editor_, &NoteEditor::mediaChanged, this, &QmlNoteEditor::mediaChanged);
    connect(editor_, &NoteEditor::mediaInserted, this, &QmlNoteEditor::mediaInserted);
    connect(editor_, &NoteEditor::documentLoaded, this, [this](bool modeChanged, bool formatConversion) {
        const quint64 generation  = ++loadGeneration_;
        suppressNextFocusRefresh_ = formatConversion;
        QTimer::singleShot(0, this, [this, generation, modeChanged, formatConversion] {
            if (generation != loadGeneration_)
                return;
            // A containing NoteWidget can suppress signals while loading. Emit
            // the derived UI state after that synchronous scope has ended.
            emit undoStateChanged();
            if (modeChanged)
                emit formatChanged(model_->markdown());
            if (formatConversion) {
                emit contentsChanged();
                emit focusLost();
            }
        });
    });
    connect(editor_, &NoteEditor::historyDocumentRestored, this, [this](bool modeChanged) {
        if (modeChanged)
            emit formatChanged(model_->markdown());
    });
    connect(model_, &NoteBlockModel::contentsChanged, this, &QmlNoteEditor::contentsChanged);
    updateFocusWindow();
}

QmlNoteEditor::~QmlNoteEditor()
{
    // Tear down the QML tree while this context object still has a complete
    // meta-object. Otherwise late binding updates can attempt to call into a
    // QObject that is already in its base-class destructor.
    if (quick_)
        quick_->setSource(QUrl());
}

void QmlNoteEditor::flushPendingEditorChanges()
{
    if (quick_ && quick_->rootObject())
        QMetaObject::invokeMethod(quick_->rootObject(), "flushPendingEditorChanges");
}

void QmlNoteEditor::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    updateFocusWindow();
}

void QmlNoteEditor::updateFocusWindow()
{
    QWidget *current = window();
    if (focusWindow_ == current)
        return;
    if (focusWindow_)
        focusWindow_->removeEventFilter(this);
    focusWindow_ = current;
    if (focusWindow_ && focusWindow_ != this)
        focusWindow_->installEventFilter(this);
}

void QmlNoteEditor::load(const QString &contents, Note::Format format, NoteEditor::LoadPolicy policy)
{
    editor_->loadDocument(contents, format, policy);
}

void QmlNoteEditor::setMedia(const QList<MediaReference> &media) { editor_->setMedia(media); }

QList<MediaReference> QmlNoteEditor::media() const { return editor_->media(); }

void QmlNoteEditor::saveImageAs(const QString &url)
{
    const MediaReference *reference = nullptr;
    const auto            media     = editor_->media();
    for (const auto &candidate : media) {
        if (candidate.uri() == url) {
            reference = &candidate;
            break;
        }
    }
    if (!reference) {
        QMessageBox::warning(this, tr("Save Image As"), tr("The image data is not available locally."));
        return;
    }

    const auto loaded = LocalMediaStore::instance()->data(reference->blobId);
    if (!loaded) {
        QMessageBox::warning(this, tr("Save Image As"), tr("Could not read the image: %1").arg(loaded.error));
        return;
    }

    const QString name        = reference->portableName.isEmpty() ? reference->originalName : reference->portableName;
    const QString initialPath = QDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).filePath(name);
    const QString fileName
        = QFileDialog::getSaveFileName(this, tr("Save Image As"), initialPath,
                                       tr("Images (*.png *.jpg *.jpeg *.gif *.webp *.bmp *.svg);;All files (*)"));
    if (fileName.isEmpty())
        return;

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly) || file.write(loaded.value) != loaded.value.size() || !file.commit()) {
        QMessageBox::warning(this, tr("Save Image As"), tr("Could not save the image: %1").arg(file.errorString()));
    }
}

QString QmlNoteEditor::materializeDragImage(const MediaReference &reference, const QByteArray &data)
{
    if (data.isEmpty())
        return {};
    if (!dragExportDirectory_) {
        dragExportDirectory_
            = std::make_unique<QTemporaryDir>(QDir::tempPath() + QStringLiteral("/qtnote-image-drag-XXXXXX"));
    }
    if (!dragExportDirectory_->isValid())
        return {};

    QString name
        = QFileInfo(reference.portableName.isEmpty() ? reference.originalName : reference.portableName).fileName();
    if (name.isEmpty())
        name = QStringLiteral("image");
    const QString subdirectory
        = QDir(dragExportDirectory_->path()).filePath(reference.id.toString(QUuid::WithoutBraces));
    if (!QDir().mkpath(subdirectory))
        return {};
    const QString path = QDir(subdirectory).filePath(name);
    QSaveFile     file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
        return {};
    return path;
}

bool QmlNoteEditor::startImageDrag(int row)
{
    if (!model_->markdown() || model_->blockTypeAt(row) != int(NoteBlockModel::Image))
        return false;

    NoteFragment fragment = withMedia(model_->extractBlockFragment(row, row));
    if (!isUsableImageFragment(fragment))
        return false;

    NoteTransferController controller;
    auto                   exported = controller.createMimeData(fragment);
    if (!exported) {
        qWarning() << "QML image drag export failed:" << exported.error;
        return false;
    }

    QByteArray            imageData;
    const MediaReference *reference = nullptr;
    if (!fragment.media.isEmpty()) {
        reference         = &fragment.media.constFirst().reference;
        const auto loaded = LocalMediaStore::instance()->data(reference->blobId);
        if (loaded)
            imageData = loaded.value;
    }

    if (reference && !imageData.isEmpty()) {
        const QString exportedFile = materializeDragImage(*reference, imageData);
        if (!exportedFile.isEmpty()) {
            QList<QUrl> urls = exported.mimeData->urls();
            urls.prepend(QUrl::fromLocalFile(exportedFile));
            exported.mimeData->setUrls(urls);

            // Keep the private QtNote fragment pointing at qtnote-media, but
            // make every public representation independently usable by an
            // external process which happens to prefer HTML or Markdown over
            // image/png or text/uri-list.
            NoteFragment publicFragment = fragment;
            publicFragment.media.clear();
            publicFragment.blocks.first().image.sourceUri = QUrl::fromLocalFile(exportedFile).toString();
            QString       publicError;
            const QString markdown = NoteTransferController::markdownForFragment(publicFragment, &publicError);
            if (publicError.isEmpty()) {
                exported.mimeData->setData(QString::fromLatin1(NoteTransferController::MarkdownMimeType),
                                           markdown.toUtf8());
                exported.mimeData->setHtml(NoteTransferController::htmlForFragment(publicFragment));
                exported.mimeData->setText(NoteTransferController::plainTextForFragment(publicFragment));
            }
        }
    }

    QDrag drag(quick_);
    drag.setMimeData(exported.mimeData.release());
    QImage preview;
    if (!imageData.isEmpty())
        preview.loadFromData(imageData);
    if (!preview.isNull()) {
        const QImage thumbnail = preview.scaled(QSize(256, 192), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        drag.setPixmap(QPixmap::fromImage(thumbnail));
        drag.setHotSpot(QPoint(thumbnail.width() / 2, thumbnail.height() / 2));
    }
    drag.exec(Qt::CopyAction, Qt::CopyAction);
    return true;
}

int QmlNoteEditor::insertionRowAt(const QPointF &position) const
{
    if (!quick_ || !quick_->rootObject())
        return model_->rowCount();
    QVariant result;
    if (!QMetaObject::invokeMethod(quick_->rootObject(), "insertionRowAtPoint", Q_RETURN_ARG(QVariant, result),
                                   Q_ARG(QVariant, position.x()), Q_ARG(QVariant, position.y()))) {
        return model_->rowCount();
    }
    return qBound(0, result.toInt(), model_->rowCount());
}

bool QmlNoteEditor::canAcceptImageDrop(const QMimeData *mimeData) const
{
    if (!imageInsertionEnabled_ || !model_->markdown() || !mimeData)
        return false;
    NoteTransferController controller;
    const auto             imported = controller.importMimeData(mimeData);
    return (imported && (imported.hasImage() || isUsableImageFragment(imported.fragment)))
        || !localImageFiles(mimeData).isEmpty();
}

bool QmlNoteEditor::handleImageDrop(const QMimeData *mimeData, int row)
{
    if (!canAcceptImageDrop(mimeData))
        return false;

    NoteTransferController controller;
    const auto             imported = controller.importMimeData(mimeData);
    if (imported && isUsableImageFragment(imported.fragment)) {
        NoteFragment          fragment = imported.fragment;
        QList<MediaReference> insertedMedia;
        if (!fragment.media.isEmpty()) {
            const auto cloned = NoteFragmentMediaTransfer::cloneForDestination(fragment, *LocalMediaStore::instance(),
                                                                               LocalMediaStore::instance());
            if (!cloned) {
                qWarning() << "QML image drop media import failed:" << cloned.error;
                return false;
            }
            fragment      = cloned.fragment;
            insertedMedia = cloned.importedMedia;
        }

        const QVariantMap beforeView = captureQmlEditorState(quick_->rootObject());
        beginHistoryTransaction(QStringLiteral("drop-image"), beforeView);
        QString    error;
        const bool inserted = model_->insertBlockFragment(qBound(0, row, model_->rowCount()), fragment, &error);
        if (inserted && !insertedMedia.isEmpty()) {
            auto manifest = media();
            manifest.append(insertedMedia);
            setMedia(manifest);
            emit mediaInserted(insertedMedia);
        }
        endHistoryTransaction(captureQmlEditorState(quick_->rootObject()));
        if (!inserted)
            qWarning() << "QML image drop insertion failed:" << error;
        return inserted;
    }

    if (imported && imported.hasImage()) {
        emit imageDropRequested(imported.image, row);
        return true;
    }

    const QStringList files = localImageFiles(mimeData);
    if (!files.isEmpty()) {
        emit imageFilesDropRequested(files, row);
        return true;
    }
    return false;
}

QString QmlNoteEditor::contents() const { return model_->contents(); }

bool QmlNoteEditor::isMarkdown() const { return model_->markdown(); }

bool QmlNoteEditor::canUndo() const { return editor_->canUndo(); }

bool QmlNoteEditor::canRedo() const { return editor_->canRedo(); }

QString QmlNoteEditor::undoText() const { return editor_->undoText(); }

QString QmlNoteEditor::redoText() const { return editor_->redoText(); }

void QmlNoteEditor::insertText(const QString &text)
{
    QVariant inserted;
    if (quick_->rootObject()
        && QMetaObject::invokeMethod(quick_->rootObject(), "insertTextAtCursor", Q_RETURN_ARG(QVariant, inserted),
                                     Q_ARG(QVariant, text))
        && inserted.toBool()) {
        return;
    }
    beginHistoryTransaction(QStringLiteral("insert-text"), captureQmlEditorState(quick_->rootObject()));
    model_->appendText(text);
    endHistoryTransaction(captureQmlEditorState(quick_->rootObject()));
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

void QmlNoteEditor::beginExternalHistoryTransaction(const QString &kind)
{
    QObject *root = quick_ ? quick_->rootObject() : nullptr;
    flushPendingEditorChanges();
    beginHistoryTransaction(kind, captureQmlEditorState(root));
}

void QmlNoteEditor::endExternalHistoryTransaction()
{
    endHistoryTransaction(captureQmlEditorState(quick_->rootObject()));
}

void QmlNoteEditor::breakHistoryMerge() { editor_->breakHistoryMerge(); }

void QmlNoteEditor::registerEditorView(QObject *view) { editor_->registerEditorView(view); }

void QmlNoteEditor::beginHistoryTransaction(const QString &kind, const QVariantMap &beforeView)
{
    editor_->beginHistoryTransaction(kind, beforeView);
}

void QmlNoteEditor::endHistoryTransaction(const QVariantMap &afterView) { editor_->endHistoryTransaction(afterView); }

void QmlNoteEditor::updateHistoryViewState(const QVariantMap &viewState, bool breakMerge)
{
    editor_->updateHistoryViewState(viewState, breakMerge);
}

bool QmlNoteEditor::undo() { return editor_->undo(); }

bool QmlNoteEditor::redo() { return editor_->redo(); }

#endif

#ifdef QTNOTE_EDITOR_CORE
#define QTNOTE_SHARED_EDITOR NoteEditor
#else
#define QTNOTE_SHARED_EDITOR QmlNoteEditor
#endif

#ifdef QTNOTE_EDITOR_CORE
void NoteEditor::loadDocument(const QString &contents, Note::Format format, LoadPolicy policy)
{
    const bool        markdown                 = format != Note::PlainText;
    const bool        modeChanged              = model_->markdown() != markdown;
    const bool        formatConversion         = policy == LoadPolicy::RecordFormatConversion && modeChanged;
    const QVariantMap beforeView               = captureEditorViewState();
    const bool        nestedHistoryTransaction = historyInTransaction();

    beginHistoryTransaction(formatConversion ? QStringLiteral("format-conversion") : QStringLiteral("load"),
                            beforeView);
    model_->load(markdown ? protectLinkLabels(contents) : contents, markdown);
    if (markdown)
        restoreProtectedMarkdown(model_);

    if (formatConversion)
        endHistoryTransaction(beforeView);
    else {
        resetContent(model_->contents(), markdown ? Note::Markdown : Note::PlainText);
        resetHistory();
    }

    if (formatConversion && !nestedHistoryTransaction)
        scheduleEditorViewRestore(beforeView);
    emit documentLoaded(modeChanged, formatConversion);
}
#endif

bool QTNOTE_SHARED_EDITOR::primaryModifierPressed() const
{
    const auto modifiers = QGuiApplication::keyboardModifiers();
    return modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::MetaModifier);
}

QVariantMap QTNOTE_SHARED_EDITOR::linkInfo(QQuickTextDocument *quickDocument, int start, int end) const
{
    QVariantMap result;
    result.insert(QStringLiteral("valid"), false);
    if (!quickDocument || !quickDocument->textDocument())
        return result;

    auto     *document = quickDocument->textDocument();
    const int limit    = documentEnd(document);
    start              = qBound(0, start, limit);
    end                = qBound(start, end, limit);

    QString href;
    if (start == end && limit > 0) {
        int             probe  = qMin(start, limit - 1);
        QTextCharFormat format = formatAt(document, probe);
        if ((!format.isAnchor() || format.anchorHref().isEmpty()) && probe > 0) {
            --probe;
            format = formatAt(document, probe);
        }
        if (format.isAnchor() && !format.anchorHref().isEmpty()) {
            href  = format.anchorHref();
            start = probe;
            while (start > 0) {
                const QTextCharFormat previous = formatAt(document, start - 1);
                if (!previous.isAnchor() || previous.anchorHref() != href)
                    break;
                --start;
            }
            end = probe + 1;
            while (end < limit) {
                const QTextCharFormat next = formatAt(document, end);
                if (!next.isAnchor() || next.anchorHref() != href)
                    break;
                ++end;
            }
        }
    } else if (start < end) {
        const QTextCharFormat first = formatAt(document, start);
        if (first.isAnchor() && !first.anchorHref().isEmpty()) {
            const QString candidate = first.anchorHref();
            bool          sameLink  = true;
            for (int position = start + 1; position < end; ++position) {
                const QTextCharFormat format = formatAt(document, position);
                if (!format.isAnchor() || format.anchorHref() != candidate) {
                    sameLink = false;
                    break;
                }
            }
            if (sameLink)
                href = candidate;
        }
    }

    result.insert(QStringLiteral("valid"), true);
    result.insert(QStringLiteral("start"), start);
    result.insert(QStringLiteral("end"), end);
    result.insert(QStringLiteral("href"), href);
    return result;
}

int QTNOTE_SHARED_EDITOR::setLink(QQuickTextDocument *quickDocument, int start, int end, const QString &href)
{
    if (!quickDocument || !quickDocument->textDocument())
        return -1;

    auto     *document = quickDocument->textDocument();
    const int limit    = documentEnd(document);
    start              = qBound(0, start, limit);
    end                = qBound(start, end, limit);

    QTextCursor cursor(document);
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    if (!cursor.hasSelection() && !href.isEmpty()) {
        const QString placeholder = tr("link");
        cursor.insertText(placeholder);
        end = start + placeholder.size();
    }

    if (start < end)
        setLinkOnRange(document, start, end, href);
    return end;
}

int QTNOTE_SHARED_EDITOR::applyInlineFormat(QQuickTextDocument *quickDocument, int start, int end, const QString &style)
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
    else if (style == QLatin1String("underline"))
        format.setFontUnderline(!current.fontUnderline());
    else if (style == QLatin1String("code")) {
        format.setFontFixedPitch(!current.fontFixedPitch());
        if (!current.fontFixedPitch()) {
            format.setFontFamilies({ QStringLiteral("monospace") });
        } else {
            format.setFontFamilies(document->defaultFont().families());
        }
    } else if (style == QLatin1String("link")) {
        format.setAnchor(!current.isAnchor());
        format.setAnchorHref(current.isAnchor() ? QString() : QStringLiteral("url"));
    } else {
        return -1;
    }
    cursor.mergeCharFormat(format);
    return end;
}

void QTNOTE_SHARED_EDITOR::applyInlineHtmlFormatting(QQuickTextDocument *quickDocument) const
{
    if (!quickDocument || !quickDocument->textDocument())
        return;

    static const QString openMarker  = QStringLiteral("QTNOTEINSOPEN7F3A");
    static const QString closeMarker = QStringLiteral("QTNOTEINSCLOSE7F3A");
    auto                *document    = quickDocument->textDocument();
    QTextCursor          search(document);
    while (true) {
        QTextCursor open = document->find(openMarker, search);
        if (open.isNull())
            break;
        QTextCursor close = document->find(closeMarker, open);
        if (close.isNull())
            break;

        const int start = open.selectionStart();
        const int end   = close.selectionStart() - openMarker.size();
        close.removeSelectedText();
        open.removeSelectedText();

        QTextCursor content(document);
        content.setPosition(start);
        content.setPosition(qMax(start, end), QTextCursor::KeepAnchor);
        QTextCharFormat underline;
        underline.setFontUnderline(true);
        content.mergeCharFormat(underline);
        search.setPosition(qMax(start, end));
    }
}

QString QTNOTE_SHARED_EDITOR::markdownText(QQuickTextDocument *quickDocument) const
{
    if (!quickDocument || !quickDocument->textDocument())
        return {};

    auto   *document = quickDocument->textDocument();
    QString markdown = unwrapMarkdownWriterLines(markdownWithSerializedInlineFormats(document));
    while (markdown.endsWith(QLatin1Char('\n')) || markdown.endsWith(QLatin1Char('\r')))
        markdown.chop(1);
    return markdown;
}

QString QTNOTE_SHARED_EDITOR::markdownTableCellText(QQuickTextDocument *quickDocument) const
{
    if (!quickDocument || !quickDocument->textDocument())
        return {};

    // QTextDocument serializes a visual line separator as an ordinary
    // newline, indistinguishable from its own soft wrapping.  Protect every
    // intentional separator before invoking the normal Markdown serializer,
    // then restore it using the table-safe GFM spelling.
    std::unique_ptr<QTextDocument> document(quickDocument->textDocument()->clone());
    const QString                  marker = QStringLiteral("QTNOTETABLEHARDBREAK8C53");
    for (int position = document->characterCount() - 2; position >= 0; --position) {
        const QChar character = document->characterAt(position);
        if (character != QChar::LineSeparator && character != QChar::ParagraphSeparator)
            continue;
        QTextCursor cursor(document.get());
        cursor.setPosition(position);
        cursor.setPosition(position + 1, QTextCursor::KeepAnchor);
        cursor.insertText(marker, QTextCharFormat());
    }

    QString markdown = unwrapMarkdownWriterLines(markdownWithSerializedInlineFormats(document.get()));
    while (markdown.endsWith(QLatin1Char('\n')) || markdown.endsWith(QLatin1Char('\r')))
        markdown.chop(1);
    markdown.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    markdown.replace(marker, QStringLiteral("<br>"));
    return markdown;
}

QString QTNOTE_SHARED_EDITOR::markdownSelection(QQuickTextDocument *quickDocument, int start, int end) const
{
    if (!quickDocument || !quickDocument->textDocument())
        return {};
    return markdownRange(quickDocument->textDocument(), start, end);
}

#ifndef QTNOTE_EDITOR_CORE
void QmlNoteEditor::registerTextDocument(QQuickTextDocument *document, bool titleDocument)
{
    if (!document || !document->textDocument())
        return;
    document->textDocument()->setUndoRedoEnabled(false);
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

    auto      *textDocument      = document->textDocument();
    const auto visibleTextIsHref = [textDocument](int position) {
        const int limit = documentEnd(textDocument);
        if (position < 0 || position >= limit)
            return false;

        const QTextCharFormat format = formatAt(textDocument, position);
        const QString         href   = format.anchorHref().trimmed();
        if (!format.isAnchor() || href.isEmpty())
            return false;

        int start = position;
        while (start > 0) {
            const QTextCharFormat previous = formatAt(textDocument, start - 1);
            if (!previous.isAnchor() || previous.anchorHref() != href)
                break;
            --start;
        }

        int end = position + 1;
        while (end < limit) {
            const QTextCharFormat next = formatAt(textDocument, end);
            if (!next.isAnchor() || next.anchorHref() != href)
                break;
            ++end;
        }

        QTextCursor cursor(textDocument);
        cursor.setPosition(start);
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        QString visible = cursor.selectedText().trimmed();
        visible.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));

        return visible == href || (href.startsWith(QStringLiteral("mailto:")) && visible == href.mid(7))
            || (visible.startsWith(QStringLiteral("www.")) && href == QStringLiteral("https://") + visible);
    };

    for (auto block = textDocument->begin(); block.isValid(); block = block.next()) {
        if (!block.layout())
            continue;
        for (const auto &range : block.layout()->formats()) {
            if (!range.format.property(SpellCheckFormatProperty).toBool())
                continue;

            const int start = block.position() + range.start;
            if (visibleTextIsHref(start))
                continue;

            QVariantMap value;
            value.insert(QStringLiteral("start"), start);
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
#endif

NoteFragment QTNOTE_SHARED_EDITOR::documentFragment() const
{
    return withMedia(model_->extractBlockFragment(0, model_->rowCount() - 1));
}

NoteFragment QTNOTE_SHARED_EDITOR::withMedia(NoteFragment fragment) const
{
    for (const NoteFragmentBlock &block : std::as_const(fragment.blocks)) {
        if (block.type != NoteFragmentBlockType::Image)
            continue;
        for (const MediaReference &reference : media()) {
            if (reference.isValid() && reference.uri() == block.image.sourceUri) {
                NoteFragmentMedia media;
                media.sourceUri = block.image.sourceUri;
                media.reference = reference;
                fragment.media.append(media);
                break;
            }
        }
    }

    // A single copied image is also useful outside QtNote.  Keep its PNG data
    // reasonably bounded; larger images retain their internal blob reference.
    if (fragment.blocks.size() == 1 && fragment.blocks.constFirst().type == NoteFragmentBlockType::Image
        && fragment.media.size() == 1
        && fragment.media.constFirst().reference.size <= NoteTransferController::PortableImageDataLimit) {
        const auto data = LocalMediaStore::instance()->data(fragment.media.constFirst().reference.blobId);
        if (data)
            fragment.media.first().data = data.value;
    }
    return fragment;
}

void QTNOTE_SHARED_EDITOR::copyToClipboard(const QString &text)
{
    NoteFragment fragment;
    fragment.sourceFormat = NoteFragmentSourceFormat::PlainText;
    NoteFragmentBlock block;
    block.type     = NoteFragmentBlockType::Text;
    block.markdown = text;
    fragment.blocks.append(block);

    NoteTransferController controller;
    auto                   exported = controller.createMimeData(fragment);
    if (exported)
        QGuiApplication::clipboard()->setMimeData(exported.mimeData.release());
    else
        QGuiApplication::clipboard()->setText(text);
}

void QTNOTE_SHARED_EDITOR::copyMarkdownToClipboard(const QString &markdown)
{
    // The visible selection is Markdown, not one literal text block.  Keeping
    // it as a Text block would make our private MIME format treat list/table
    // syntax as ordinary characters on the next QtNote paste.  Parse it back
    // through the block model so the private and public representations carry
    // the same structure.
    NoteBlockModel model;
    model.load(markdown, true);
    NoteFragment fragment = model.extractBlockFragment(0, model.rowCount() - 1);
    fragment.sourceFormat = NoteFragmentSourceFormat::Markdown;

    NoteTransferController controller;
    auto                   exported = controller.createMimeData(fragment);
    if (exported) {
        QGuiApplication::clipboard()->setMimeData(exported.mimeData.release());
        qInfo() << "QML clipboard copy: selection blocks=" << fragment.blocks.size()
                << "formats=" << QGuiApplication::clipboard()->mimeData()->formats();
    } else {
        QGuiApplication::clipboard()->setText(markdown);
        qWarning() << "QML clipboard copy fell back to plain text:" << exported.error;
    }
}

void QTNOTE_SHARED_EDITOR::copyDocumentToClipboard()
{
    NoteTransferController controller;
    const NoteFragment     fragment = documentFragment();
    auto                   exported = controller.createMimeData(fragment);
    if (exported) {
        QGuiApplication::clipboard()->setMimeData(exported.mimeData.release());
        qInfo() << "QML clipboard copy: whole document blocks=" << fragment.blocks.size()
                << "formats=" << QGuiApplication::clipboard()->mimeData()->formats();
    } else {
        QGuiApplication::clipboard()->setText(model_->contents());
        qWarning() << "QML clipboard copy fell back to plain text:" << exported.error;
    }
}

bool QTNOTE_SHARED_EDITOR::copySelectionToClipboard(const QVariantList &encodedRanges)
{
    NoteFragment fragment = withMedia(model_->extractSelectionFragment(decodeSelectionRanges(encodedRanges)));
    if (fragment.blocks.isEmpty())
        return false;

    NoteTransferController controller;
    auto                   exported = controller.createMimeData(fragment);
    if (!exported) {
        qWarning() << "QML structured selection copy failed:" << exported.error;
        return false;
    }
    QGuiApplication::clipboard()->setMimeData(exported.mimeData.release());
    qInfo() << "QML clipboard copy: structured selection blocks=" << fragment.blocks.size()
            << "formats=" << QGuiApplication::clipboard()->mimeData()->formats();
    return true;
}

QVariantMap QTNOTE_SHARED_EDITOR::deleteSelection(const QVariantList &encodedRanges)
{
    const QList<NoteBlockSelectionRange> ranges   = decodeSelectionRanges(encodedRanges);
    const int                            focusRow = model_->removeSelectionRanges(ranges);
    if (focusRow < 0)
        return {};
    QVariantMap result { { QStringLiteral("handled"), true }, { QStringLiteral("focusRow"), focusRow } };
    // If the selection started inside an editor, its prefix survived at the
    // same structural address. Preserve the exact QTextDocument position of
    // that boundary instead of merely focusing the beginning of the block.
    if (!ranges.isEmpty() && !ranges.constFirst().before.isEmpty() && !encodedRanges.isEmpty()) {
        const QVariantMap first = encodedRanges.constFirst().toMap();
        result.insert(QStringLiteral("focusPosition"), first.value(QStringLiteral("selectionStart"), 0));
    }
    return result;
}

QVariantMap QTNOTE_SHARED_EDITOR::pasteStructuredFromClipboard(QQuickTextDocument *quickDocument, int row, int start,
                                                               int end)
{
    QVariantMap result;
    if (!model_->markdown() || !quickDocument || !quickDocument->textDocument())
        return result;

    NoteTransferController controller;
    const auto             imported = controller.importMimeData(QGuiApplication::clipboard()->mimeData());
    if (!imported || imported.sourceMimeType == QStringLiteral("text/plain") || imported.hasImage()
        || imported.fragment.blocks.isEmpty()) {
        return result;
    }

    NoteFragment          fragment = imported.fragment;
    QList<MediaReference> insertedMedia;
    if (!fragment.media.isEmpty()) {
        const auto cloned = NoteFragmentMediaTransfer::cloneForDestination(fragment, *LocalMediaStore::instance(),
                                                                           LocalMediaStore::instance());
        if (!cloned) {
            result.insert(QStringLiteral("error"), cloned.error);
            return result;
        }
        fragment      = cloned.fragment;
        insertedMedia = cloned.importedMedia;
    }

    QTextDocument *document = quickDocument->textDocument();
    const int      limit    = documentEnd(document);

    QString   error;
    const int insertedRow = model_->replaceTextBlockRangeWithFragment(
        row, markdownRange(document, 0, start), markdownRange(document, end, limit), fragment, &error);
    if (insertedRow < 0) {
        result.insert(QStringLiteral("error"), error);
        return result;
    }
    result.insert(QStringLiteral("handled"), true);
    result.insert(QStringLiteral("focusRow"), insertedRow);
    if (!insertedMedia.isEmpty()) {
        auto manifest = media();
        manifest.append(insertedMedia);
        setMedia(manifest);
        emit mediaInserted(insertedMedia);
    }
    return result;
}

QVariantMap QTNOTE_SHARED_EDITOR::pasteTableFromClipboard(int row, int cell)
{
    QVariantMap result;
    if (!model_->markdown())
        return result;
    NoteTransferController controller;
    const auto             imported = controller.importMimeData(QGuiApplication::clipboard()->mimeData());
    if (!imported || imported.sourceMimeType == QStringLiteral("text/plain") || imported.hasImage()
        || !imported.fragment.media.isEmpty() || imported.fragment.blocks.size() != 1
        || imported.fragment.blocks.constFirst().type != NoteFragmentBlockType::Table) {
        return result;
    }

    QString error;
    if (!model_->replaceTableCellsWithFragment(row, cell, imported.fragment, &error)) {
        result.insert(QStringLiteral("error"), error);
        return result;
    }
    result.insert(QStringLiteral("handled"), true);
    return result;
}

QVariantMap QTNOTE_SHARED_EDITOR::pasteListFromClipboard(QQuickTextDocument *quickDocument, int row, int item,
                                                         int start, int end)
{
    QVariantMap result;
    if (!model_->markdown() || !quickDocument || !quickDocument->textDocument())
        return result;
    NoteTransferController controller;
    const auto             imported = controller.importMimeData(QGuiApplication::clipboard()->mimeData());
    if (!imported || imported.sourceMimeType == QStringLiteral("text/plain") || imported.hasImage()
        || !imported.fragment.media.isEmpty() || imported.fragment.blocks.size() != 1
        || imported.fragment.blocks.constFirst().type != NoteFragmentBlockType::List) {
        return result;
    }

    QTextDocument *document = quickDocument->textDocument();
    const int      limit    = documentEnd(document);
    QString        error;
    const int      focusItem = model_->replaceListItemRangeWithFragment(
        row, item, markdownRange(document, 0, start), markdownRange(document, end, limit), imported.fragment, &error);
    if (focusItem < 0) {
        result.insert(QStringLiteral("error"), error);
        return result;
    }
    result.insert(QStringLiteral("handled"), true);
    result.insert(QStringLiteral("focusItem"), focusItem);
    return result;
}

#undef QTNOTE_SHARED_EDITOR

#ifndef QTNOTE_EDITOR_CORE
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
    if (watched == focusWindow_ && event->type() == QEvent::WindowDeactivate)
        emit focusLost();

    if (watched == quick_) {
        if (event->type() == QEvent::DragEnter) {
            auto *dragEvent    = static_cast<QDragEnterEvent *>(event);
            imageDragAccepted_ = canAcceptImageDrop(dragEvent->mimeData());
            if (imageDragAccepted_) {
                dragEvent->setDropAction(Qt::CopyAction);
                dragEvent->accept();
                return true;
            }
        } else if (event->type() == QEvent::DragMove && imageDragAccepted_) {
            auto *dragEvent = static_cast<QDragMoveEvent *>(event);
            dragEvent->setDropAction(Qt::CopyAction);
            dragEvent->accept();
            return true;
        } else if (event->type() == QEvent::DragLeave) {
            imageDragAccepted_ = false;
        } else if (event->type() == QEvent::Drop && imageDragAccepted_) {
            auto *dropEvent = static_cast<QDropEvent *>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            const QPointF position = dropEvent->position();
#else
            const QPointF position = dropEvent->posF();
#endif
            imageDragAccepted_ = false;
            if (handleImageDrop(dropEvent->mimeData(), insertionRowAt(position))) {
                dropEvent->setDropAction(Qt::CopyAction);
                dropEvent->accept();
                return true;
            }
            dropEvent->ignore();
            return true;
        } else if (event->type() == QEvent::KeyPress) {
            auto       keyEvent                 = static_cast<QKeyEvent *>(event);
            QObject   *root                     = quick_->rootObject();
            const bool documentHistoryOwnsFocus = invokeQmlBoolean(root, "documentHistoryOwnsFocus");
            if (documentHistoryOwnsFocus) {
                const auto modifiers = keyEvent->modifiers();
                const bool plainText = !keyEvent->text().isEmpty()
                    && !(modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier));
                const bool deletion = keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete;
                updateHistoryViewState(captureQmlEditorState(root), !(plainText || deletion));
                if (keyEvent->matches(QKeySequence::Undo) && undo())
                    return true;
                if (keyEvent->matches(QKeySequence::Redo) && redo())
                    return true;
                if (keyEvent->matches(QKeySequence::Copy) && invokeQmlBoolean(root, "copyActiveSelection"))
                    return true;
                if (keyEvent->matches(QKeySequence::Cut) && invokeQmlBoolean(root, "cutActiveSelection"))
                    return true;
                if (keyEvent->matches(QKeySequence::Paste)) {
                    const auto             mime = QGuiApplication::clipboard()->mimeData();
                    NoteTransferController controller;
                    const auto             imported = controller.importMimeData(mime);
                    qInfo() << "QML clipboard paste: formats=" << (mime ? mime->formats() : QStringList())
                            << "selected=" << imported.sourceMimeType << "blocks=" << imported.fragment.blocks.size()
                            << "image=" << imported.hasImage() << "error=" << imported.error;
                    // Office applications often publish a bitmap preview together
                    // with TSV or HTML. Image is a fallback, never a reason to
                    // discard the more structured representation.
                    const bool hasStructuredPayload = imported && !imported.hasImage()
                        && imported.sourceMimeType != QStringLiteral("text/plain")
                        && !imported.fragment.blocks.isEmpty();
                    if (mime && mime->hasImage() && !hasStructuredPayload) {
                        const auto image = qvariant_cast<QImage>(mime->imageData());
                        if (!image.isNull()) {
                            emit imagePasteRequested(image);
                            return true;
                        }
                    }
                    // Handle the complete operation while the event still belongs
                    // to QQuickWidget. This prevents TextArea's native rich-text
                    // paste from racing the QML Keys handler.
                    if (invokeQmlBoolean(root, "pasteClipboard"))
                        return true;
                }
            }
        } else if (event->type() == QEvent::InputMethod) {
            QObject *root = quick_->rootObject();
            if (invokeQmlBoolean(root, "documentHistoryOwnsFocus"))
                updateHistoryViewState(captureQmlEditorState(root), false);
        }
        if (event->type() == QEvent::FocusIn) {
            // Retry a pending checkpoint before NoteWidget checks whether an
            // externally stored version should replace the editor contents.
            emit focusLost();
            if (suppressNextFocusRefresh_)
                suppressNextFocusRefresh_ = false;
            else
                emit focusReceived();
            QTimer::singleShot(0, this, &QmlNoteEditor::focusEditor);
        } else if (event->type() == QEvent::FocusOut)
            emit focusLost();
    }
    return QWidget::eventFilter(watched, event);
}
#endif
} // namespace QtNote
