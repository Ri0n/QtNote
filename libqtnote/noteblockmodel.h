#ifndef NOTEBLOCKMODEL_H
#define NOTEBLOCKMODEL_H

#include <QAbstractListModel>

#include "note.h"
#include "notefragment.h"

namespace QtNote {

class QTNOTE_EXPORT NoteBlockModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool markdown READ markdown NOTIFY markdownChanged)
    Q_PROPERTY(QString contents READ contents WRITE setContents NOTIFY contentsChanged)

public:
    enum BlockType { Text, BulletList, CheckList, Table, Image, NumberedList, Heading };
    Q_ENUM(BlockType)
    enum Role {
        TypeRole = Qt::UserRole + 1,
        TextRole,
        ItemsRole,
        CheckedRole,
        CellsRole,
        UrlRole,
        AltRole,
        PreviewUrlRole,
        IndentsRole,
        ItemTypesRole,
        HeadingLevelRole
    };

    explicit NoteBlockModel(QObject *parent = nullptr);

    int                    rowCount(const QModelIndex &parent = {}) const override;
    QVariant               data(const QModelIndex &index, int role) const override;
    bool                   setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags          flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool    markdown() const { return markdown_; }
    QString contents() const;
    void    setContents(const QString &contents);

    Q_INVOKABLE void load(const QString &contents, bool markdown);
    Q_INVOKABLE void setBlockText(int row, const QString &text);
    Q_INVOKABLE void setListItem(int row, int item, const QString &text);
    Q_INVOKABLE void insertListItem(int row, int item, const QString &text);
    Q_INVOKABLE void mergeListItemWithNext(int row, int item);
    Q_INVOKABLE void removeListItem(int row, int item);
    Q_INVOKABLE void removeListItems(int row, int firstItem, int lastItem);
    Q_INVOKABLE void convertListToText(int row);
    Q_INVOKABLE void indentListItems(int row, int firstItem, int lastItem, int delta);
    Q_INVOKABLE void setChecked(int row, int item, bool checked);
    Q_INVOKABLE void setTableCell(int row, int cell, const QString &text);
    Q_INVOKABLE void insertTableRow(int row, int tableRow);
    Q_INVOKABLE void removeTableRow(int row, int tableRow);
    Q_INVOKABLE void removeTableRows(int row, int firstRow, int lastRow);
    Q_INVOKABLE void insertTableColumn(int row, int column);
    Q_INVOKABLE void removeTableColumn(int row, int column);
    Q_INVOKABLE void setImageUrl(int row, const QString &url);
    Q_INVOKABLE void setImageAlt(int row, const QString &alt);
    Q_INVOKABLE void appendTextBlock();
    Q_INVOKABLE void appendText(const QString &text);
    Q_INVOKABLE void appendImage(const QString &url, const QString &alt);
    Q_INVOKABLE void insertTable(int row);
    Q_INVOKABLE void insertList(int row, BlockType type);
    Q_INVOKABLE bool convertListLevel(int row, int item, BlockType type);
    Q_INVOKABLE int  convertTextBlockToHeading(int row, int position, int level);
    Q_INVOKABLE void removeBlock(int row);
    void             setPreviewUrls(const QHash<QString, QString> &urls);

    // C++ transfer boundary.  These operations deal in whole structural
    // blocks; precise text and table-cell selections will be layered on top
    // of them by NoteTransferController.
    NoteFragment extractBlockFragment(int firstRow, int lastRow) const;
    bool         insertBlockFragment(int row, const NoteFragment &fragment, QString *error = nullptr);

signals:
    void markdownChanged();
    void contentsChanged();

private:
    struct Block {
        BlockType    type = Text;
        QString      text;
        QStringList  items;
        QVariantList indents;
        QVariantList itemTypes;
        QVariantList checked;
        QStringList  cells;
        int          columns = 0;
        QString      url;
        QString      alt;
        int          headingLevel = 0;
    };

    static QList<Block> parseMarkdown(const QString &source);
    static QString      writeMarkdown(const QList<Block> &blocks);
    void                changed(int row, const QList<int> &roles);

    QList<Block>            blocks_;
    QHash<QString, QString> previewUrls_;
    bool                    markdown_ = false;
};

} // namespace QtNote

#endif
