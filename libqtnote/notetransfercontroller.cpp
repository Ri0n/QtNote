#include "notetransfercontroller.h"

#include "noteblockmodel.h"

#include <QBuffer>
#include <QMimeData>
#include <QTextBlock>
#include <QTextDocument>
#include <QUrl>

#include <algorithm>

namespace QtNote {
namespace {

    NoteTransferController::ImportResult failed(const QString &error)
    {
        NoteTransferController::ImportResult result;
        result.error = error;
        return result;
    }

    QString markdownFromFragment(const NoteFragment &fragment, QString *error)
    {
        NoteBlockModel model;
        // NoteBlockModel defaults to plain text.  A transfer fragment is rendered
        // as GFM regardless of the source mode so that all public clipboard
        // formats preserve the structural representation.
        model.load(QString(), true);
        model.removeBlock(0);
        QString modelError;
        if (!model.insertBlockFragment(0, fragment, &modelError)) {
            if (error)
                *error = modelError;
            return {};
        }
        return model.contents();
    }

    NoteTransferController::ImportResult fragmentFromMarkdown(const QString &markdown, const QString &sourceMimeType)
    {
        NoteBlockModel model;
        model.load(markdown, true);

        NoteTransferController::ImportResult result;
        result.fragment              = model.extractBlockFragment(0, model.rowCount() - 1);
        result.fragment.sourceFormat = NoteFragmentSourceFormat::Markdown;
        result.sourceMimeType        = sourceMimeType;
        return result;
    }

    QString textForMime(const QMimeData *mime, const QString &baseType)
    {
        if (!mime)
            return {};
        if (mime->hasFormat(baseType))
            return QString::fromUtf8(mime->data(baseType));
        for (const QString &format : mime->formats()) {
            if (format.startsWith(baseType + QLatin1Char(';'), Qt::CaseInsensitive))
                return QString::fromUtf8(mime->data(format));
        }
        return {};
    }

    NoteFragment tableFragmentFromTsv(const QString &tsv)
    {
        const QStringList  lines   = tsv.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
        int                columns = 0;
        QList<QStringList> rows;
        for (QString line : lines) {
            if (line.endsWith(QLatin1Char('\r')))
                line.chop(1);
            const QStringList cells = line.split(QLatin1Char('\t'), Qt::KeepEmptyParts);
            columns                 = qMax(columns, cells.size());
            rows.append(cells);
        }

        NoteFragment      fragment;
        NoteFragmentBlock table;
        table.type             = NoteFragmentBlockType::Table;
        table.table.rows       = rows.size();
        table.table.columns    = columns;
        table.table.headerRows = rows.isEmpty() ? 0 : 1;
        for (const QStringList &sourceRow : rows) {
            for (int column = 0; column < columns; ++column)
                table.table.markdownCells.append(sourceRow.value(column));
        }
        if (table.table.rows > 0 && table.table.columns > 0)
            fragment.blocks.append(table);
        return fragment;
    }

} // namespace

NoteTransferController::ExportResult NoteTransferController::createMimeData(const NoteFragment &fragment) const
{
    ExportResult                   result;
    const QByteArray               encoded    = encodeNoteFragment(fragment);
    const NoteFragmentDecodeResult validation = decodeNoteFragment(encoded);
    if (!validation) {
        result.error = validation.error;
        return result;
    }

    QString       error;
    const QString markdown = markdownForFragment(fragment, &error);
    if (!error.isEmpty()) {
        result.error = error;
        return result;
    }
    const QString plainText = plainTextForFragment(fragment, &error);
    if (!error.isEmpty()) {
        result.error = error;
        return result;
    }
    const QString html = htmlForFragment(fragment, &error);
    if (!error.isEmpty()) {
        result.error = error;
        return result;
    }

    auto mime = std::make_unique<QMimeData>();
    mime->setData(QString::fromLatin1(FragmentMimeType), encoded);
    mime->setData(QString::fromLatin1(MarkdownMimeType), markdown.toUtf8());
    mime->setHtml(html);
    mime->setText(plainText);

    const QString tsv = tsvForFragment(fragment);
    if (!tsv.isNull())
        mime->setData(QString::fromLatin1(TsvMimeType), tsv.toUtf8());

    if (fragment.blocks.size() == 1 && fragment.blocks.constFirst().type == NoteFragmentBlockType::Image) {
        const auto media = std::find_if(fragment.media.cbegin(), fragment.media.cend(), [&fragment](const auto &item) {
            return item.sourceUri == fragment.blocks.constFirst().image.sourceUri;
        });
        if (media != fragment.media.cend() && !media->data.isEmpty()) {
            QImage image;
            image.loadFromData(media->data);
            if (!image.isNull()) {
                mime->setImageData(image);
                QBuffer buffer;
                buffer.open(QIODevice::WriteOnly);
                if (image.save(&buffer, "PNG"))
                    mime->setData(QStringLiteral("image/png"), buffer.data());
            }
        }
    }

    QList<QUrl> urls;
    for (const NoteFragmentBlock &block : fragment.blocks) {
        if (block.type != NoteFragmentBlockType::Image)
            continue;
        const QUrl url(block.image.sourceUri);
        if (url.isValid() && url.scheme().compare(QStringLiteral("qtnote-media"), Qt::CaseInsensitive) != 0)
            urls.append(url);
    }
    if (!urls.isEmpty())
        mime->setUrls(urls);

    result.mimeData = std::move(mime);
    return result;
}

NoteTransferController::ImportResult NoteTransferController::importMimeData(const QMimeData *mimeData) const
{
    if (!mimeData)
        return failed(QStringLiteral("clipboard data is unavailable"));

    if (mimeData->hasFormat(QString::fromLatin1(FragmentMimeType))) {
        const NoteFragmentDecodeResult decoded
            = decodeNoteFragment(mimeData->data(QString::fromLatin1(FragmentMimeType)));
        if (decoded) {
            ImportResult result;
            result.fragment       = decoded.fragment;
            result.sourceMimeType = QString::fromLatin1(FragmentMimeType);
            return result;
        }
        // A malformed private MIME payload must not stop an ordinary external
        // fallback representation on the same clipboard from being pasted.
    }

    const QString markdown = textForMime(mimeData, QString::fromLatin1(MarkdownMimeType));
    if (!markdown.isNull())
        return fragmentFromMarkdown(markdown, QString::fromLatin1(MarkdownMimeType));

    if (mimeData->hasHtml()) {
        QTextDocument document;
        document.setHtml(mimeData->html());
        return fragmentFromMarkdown(document.toMarkdown(QTextDocument::MarkdownDialectGitHub),
                                    QStringLiteral("text/html"));
    }

    const QString tsv = textForMime(mimeData, QString::fromLatin1(TsvMimeType));
    if (!tsv.isNull()) {
        ImportResult result;
        result.fragment       = tableFragmentFromTsv(tsv);
        result.sourceMimeType = QString::fromLatin1(TsvMimeType);
        if (result.fragment.blocks.isEmpty())
            result.error = QStringLiteral("TSV contains no table cells");
        return result;
    }

    if (mimeData->hasImage()) {
        const QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (!image.isNull()) {
            ImportResult result;
            result.image          = image;
            result.sourceMimeType = QStringLiteral("image/*");
            return result;
        }
    }

    if (mimeData->hasUrls()) {
        QStringList urls;
        for (const QUrl &url : mimeData->urls())
            if (url.isValid())
                urls.append(url.toDisplayString());
        if (!urls.isEmpty()) {
            ImportResult result = fragmentFromMarkdown(urls.join(QLatin1Char('\n')), QStringLiteral("text/uri-list"));
            return result;
        }
    }

    if (mimeData->hasText())
        return fragmentFromMarkdown(mimeData->text(), QStringLiteral("text/plain"));
    return failed(QStringLiteral("clipboard format is not supported"));
}

QString NoteTransferController::markdownForFragment(const NoteFragment &fragment, QString *error)
{
    if (error)
        error->clear();
    return markdownFromFragment(fragment, error);
}

QString NoteTransferController::plainTextForFragment(const NoteFragment &fragment, QString *error)
{
    if (fragment.sourceFormat == NoteFragmentSourceFormat::PlainText) {
        if (error)
            error->clear();
        QStringList blocks;
        for (const NoteFragmentBlock &block : fragment.blocks)
            blocks.append(block.markdown);
        return blocks.join(QStringLiteral("\n\n"));
    }
    const QString markdown = markdownForFragment(fragment, error);
    if (error && !error->isEmpty())
        return {};
    QTextDocument document;
    document.setMarkdown(markdown, QTextDocument::MarkdownDialectGitHub);
    QStringList paragraphs;
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next())
        paragraphs.append(block.text());
    return paragraphs.join(QStringLiteral("\n\n"));
}

QString NoteTransferController::htmlForFragment(const NoteFragment &fragment, QString *error)
{
    const QString markdown = markdownForFragment(fragment, error);
    if (error && !error->isEmpty())
        return {};
    QTextDocument document;
    document.setMarkdown(markdown, QTextDocument::MarkdownDialectGitHub);
    return document.toHtml();
}

QString NoteTransferController::tsvForFragment(const NoteFragment &fragment)
{
    if (fragment.blocks.size() != 1 || fragment.blocks.constFirst().type != NoteFragmentBlockType::Table)
        return QString();
    const NoteFragmentTable &table = fragment.blocks.constFirst().table;
    if (table.rows < 1 || table.columns < 1 || table.rows * table.columns != table.markdownCells.size())
        return {};

    QStringList rows;
    for (int row = 0; row < table.rows; ++row) {
        QStringList columns;
        for (int column = 0; column < table.columns; ++column) {
            QString cell = table.markdownCells.at(row * table.columns + column);
            cell.replace(QLatin1Char('\t'), QLatin1Char(' '));
            cell.replace(QLatin1Char('\n'), QStringLiteral(" "));
            columns.append(cell);
        }
        rows.append(columns.join(QLatin1Char('\t')));
    }
    return rows.join(QLatin1Char('\n'));
}

} // namespace QtNote
