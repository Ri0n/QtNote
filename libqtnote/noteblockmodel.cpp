#include "noteblockmodel.h"

#include <QRegularExpression>
#include <QTextDocument>

namespace QtNote {
namespace {
    QStringList tableCells(QString line)
    {
        line = line.trimmed();
        if (line.startsWith('|'))
            line.remove(0, 1);
        if (line.endsWith('|'))
            line.chop(1);
        auto cells = line.split('|');
        for (auto &cell : cells)
            cell = cell.trimmed();
        return cells;
    }

    bool isTableSeparator(const QString &line)
    {
        static const QRegularExpression separator(
            QStringLiteral(R"(^\s*\|?\s*:?-{3,}:?\s*(\|\s*:?-{3,}:?\s*)+\|?\s*$)"));
        return separator.match(line).hasMatch();
    }
}

NoteBlockModel::NoteBlockModel(QObject *parent) : QAbstractListModel(parent) { }

int NoteBlockModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : blocks_.size(); }

QVariant NoteBlockModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= blocks_.size())
        return {};
    const auto &block = blocks_.at(index.row());
    switch (role) {
    case TypeRole:
        return block.type;
    case TextRole:
        return block.text;
    case ItemsRole:
        return block.items;
    case CheckedRole:
        return block.checked;
    case CellsRole: {
        QVariantMap table;
        table.insert(QStringLiteral("values"), block.cells);
        table.insert(QStringLiteral("columns"), block.columns);
        return table;
    }
    case UrlRole:
        return block.url;
    case AltRole:
        return block.alt;
    case PreviewUrlRole:
        return previewUrls_.value(block.url, block.url);
    default:
        return {};
    }
}

bool NoteBlockModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= blocks_.size())
        return false;
    auto &block = blocks_[index.row()];
    switch (role) {
    case TextRole:
        block.text = value.toString();
        break;
    case ItemsRole:
        block.items = value.toStringList();
        break;
    case CheckedRole:
        block.checked = value.toList();
        break;
    case UrlRole:
        block.url = value.toString();
        break;
    case AltRole:
        block.alt = value.toString();
        break;
    default:
        return false;
    }
    changed(index.row(), { role });
    return true;
}

Qt::ItemFlags NoteBlockModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index) | (index.isValid() ? Qt::ItemIsEditable : Qt::NoItemFlags);
}

QHash<int, QByteArray> NoteBlockModel::roleNames() const
{
    return { { TypeRole, "blockType" }, { TextRole, "blockText" },
             { ItemsRole, "items" },    { CheckedRole, "checkedItems" },
             { CellsRole, "table" },    { UrlRole, "url" },
             { AltRole, "alt" },        { PreviewUrlRole, "previewUrl" } };
}

QString NoteBlockModel::contents() const
{
    if (markdown_)
        return writeMarkdown(blocks_).trimmed();
    return blocks_.isEmpty() ? QString() : blocks_.constFirst().text;
}

void NoteBlockModel::setContents(const QString &contents) { load(contents, markdown_); }

void NoteBlockModel::load(const QString &contents, bool markdown)
{
    beginResetModel();
    markdown_ = markdown;
    blocks_   = markdown ? parseMarkdown(contents) : QList<Block> { Block { Text, contents } };
    endResetModel();
    emit markdownChanged();
    emit contentsChanged();
}

void NoteBlockModel::setBlockText(int row, const QString &text) { setData(index(row), text, TextRole); }

void NoteBlockModel::setListItem(int row, int item, const QString &text)
{
    if (row < 0 || row >= blocks_.size() || item < 0 || item >= blocks_[row].items.size())
        return;
    blocks_[row].items[item] = text;
    changed(row, { ItemsRole });
}

void NoteBlockModel::setChecked(int row, int item, bool checked)
{
    if (row < 0 || row >= blocks_.size() || item < 0 || item >= blocks_[row].checked.size())
        return;
    blocks_[row].checked[item] = checked;
    changed(row, { CheckedRole });
}

void NoteBlockModel::setTableCell(int row, int cell, const QString &text)
{
    if (row < 0 || row >= blocks_.size() || cell < 0 || cell >= blocks_[row].cells.size())
        return;
    blocks_[row].cells[cell] = text;
    changed(row, { CellsRole });
}

void NoteBlockModel::setImageUrl(int row, const QString &url) { setData(index(row), url, UrlRole); }
void NoteBlockModel::setImageAlt(int row, const QString &alt) { setData(index(row), alt, AltRole); }

void NoteBlockModel::appendTextBlock()
{
    const int row = blocks_.size();
    beginInsertRows({}, row, row);
    blocks_.append(Block {});
    endInsertRows();
    emit contentsChanged();
}

void NoteBlockModel::appendText(const QString &text)
{
    if (!blocks_.isEmpty() && blocks_.last().type == Text) {
        auto &value = blocks_.last().text;
        if (!value.isEmpty() && !value.back().isSpace())
            value += QLatin1Char(' ');
        value += text;
        changed(blocks_.size() - 1, { TextRole });
        return;
    }
    const int row = blocks_.size();
    beginInsertRows({}, row, row);
    Block block;
    block.text = text;
    blocks_.append(block);
    endInsertRows();
    emit contentsChanged();
}

void NoteBlockModel::appendImage(const QString &url, const QString &alt)
{
    const int row = blocks_.size();
    beginInsertRows({}, row, row);
    Block block;
    block.type = Image;
    block.url  = url;
    block.alt  = alt;
    blocks_.append(block);
    endInsertRows();
    emit contentsChanged();
}

void NoteBlockModel::removeBlock(int row)
{
    if (row < 0 || row >= blocks_.size())
        return;
    beginRemoveRows({}, row, row);
    blocks_.removeAt(row);
    endRemoveRows();
    emit contentsChanged();
}

void NoteBlockModel::setPreviewUrls(const QHash<QString, QString> &urls)
{
    previewUrls_ = urls;
    if (!blocks_.isEmpty())
        emit dataChanged(index(0), index(blocks_.size() - 1), { PreviewUrlRole });
}

QList<NoteBlockModel::Block> NoteBlockModel::parseMarkdown(const QString &source)
{
    // QTextDocument is the Markdown reader. Parsing its canonical Markdown keeps
    // Qt's CommonMark interpretation as the single source of truth.
    QTextDocument document;
    document.setMarkdown(source, QTextDocument::MarkdownDialectGitHub);
    const auto                      lines = document.toMarkdown(QTextDocument::MarkdownDialectGitHub).split('\n');
    QList<Block>                    result;
    static const QRegularExpression task(QStringLiteral(R"(^\s*[-*+]\s+\[([ xX])\]\s+(.*)$)"));
    static const QRegularExpression bullet(QStringLiteral(R"(^\s*[-*+]\s+(.*)$)"));
    static const QRegularExpression image(QStringLiteral(R"(^\s*!\[([^\]]*)\]\((\S+?)(?:\s+"[^"]*")?\)\s*$)"));

    for (int i = 0; i < lines.size();) {
        if (lines[i].trimmed().isEmpty()) {
            ++i;
            continue;
        }
        auto match = task.match(lines[i]);
        if (match.hasMatch()) {
            Block block;
            block.type = CheckList;
            while (i < lines.size() && (match = task.match(lines[i])).hasMatch()) {
                block.checked.append(match.captured(1).compare(QStringLiteral("x"), Qt::CaseInsensitive) == 0);
                block.items.append(match.captured(2));
                ++i;
            }
            result.append(block);
            continue;
        }
        match = bullet.match(lines[i]);
        if (match.hasMatch()) {
            Block block;
            block.type = BulletList;
            while (i < lines.size() && !task.match(lines[i]).hasMatch()
                   && (match = bullet.match(lines[i])).hasMatch()) {
                block.items.append(match.captured(1));
                ++i;
            }
            result.append(block);
            continue;
        }
        if (i + 1 < lines.size() && lines[i].contains('|') && isTableSeparator(lines[i + 1])) {
            Block block;
            block.type    = Table;
            auto header   = tableCells(lines[i]);
            block.columns = header.size();
            block.cells.append(header);
            i += 2;
            while (i < lines.size() && lines[i].contains('|') && !lines[i].trimmed().isEmpty()) {
                auto row = tableCells(lines[i++]);
                while (row.size() < block.columns)
                    row.append(QString());
                block.cells.append(row.mid(0, block.columns));
            }
            result.append(block);
            continue;
        }
        match = image.match(lines[i]);
        if (match.hasMatch()) {
            Block block;
            block.type = Image;
            block.alt  = match.captured(1);
            block.url  = match.captured(2);
            result.append(block);
            ++i;
            continue;
        }
        Block block;
        block.type = Text;
        QStringList paragraph;
        while (i < lines.size() && !lines[i].trimmed().isEmpty() && !task.match(lines[i]).hasMatch()
               && !bullet.match(lines[i]).hasMatch() && !image.match(lines[i]).hasMatch()
               && !(i + 1 < lines.size() && lines[i].contains('|') && isTableSeparator(lines[i + 1])))
            paragraph.append(lines[i++]);
        block.text = paragraph.join('\n');
        result.append(block);
    }
    if (result.isEmpty())
        result.append(Block {});
    return result;
}

QString NoteBlockModel::writeMarkdown(const QList<Block> &blocks)
{
    QStringList output;
    for (const auto &block : blocks) {
        QString value;
        switch (block.type) {
        case Text:
            value = block.text;
            break;
        case BulletList:
            for (const auto &item : block.items)
                value += QStringLiteral("- %1\n").arg(item);
            value.chop(value.endsWith('\n') ? 1 : 0);
            break;
        case CheckList:
            for (int i = 0; i < block.items.size(); ++i)
                value += QStringLiteral("- [%1] %2\n").arg(block.checked.value(i).toBool() ? "x" : " ", block.items[i]);
            value.chop(value.endsWith('\n') ? 1 : 0);
            break;
        case Table:
            for (int row = 0; row * block.columns < block.cells.size(); ++row) {
                value += QLatin1String("| ");
                for (int col = 0; col < block.columns; ++col)
                    value += block.cells.value(row * block.columns + col) + QLatin1String(" | ");
                value.chop(1);
                value += QLatin1Char('\n');
                if (row == 0) {
                    value += QLatin1String("|");
                    for (int col = 0; col < block.columns; ++col)
                        value += QLatin1String(" --- |");
                    value += QLatin1Char('\n');
                }
            }
            value = value.trimmed();
            break;
        case Image:
            value = QStringLiteral("![%1](%2)").arg(block.alt, block.url);
            break;
        }
        output.append(value);
    }
    return output.join(QStringLiteral("\n\n"));
}

void NoteBlockModel::changed(int row, const QList<int> &roles)
{
    emit dataChanged(index(row), index(row), roles);
    emit contentsChanged();
}
} // namespace QtNote
