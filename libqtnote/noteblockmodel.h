#ifndef NOTEBLOCKMODEL_H
#define NOTEBLOCKMODEL_H

#include <QAbstractListModel>

#include "note.h"

namespace QtNote {

class QTNOTE_EXPORT NoteBlockModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool markdown READ markdown NOTIFY markdownChanged)
    Q_PROPERTY(QString contents READ contents WRITE setContents NOTIFY contentsChanged)

public:
    enum BlockType { Text, BulletList, CheckList, Table, Image };
    Q_ENUM(BlockType)
    enum Role { TypeRole = Qt::UserRole + 1, TextRole, ItemsRole, CheckedRole, CellsRole, UrlRole, AltRole };

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
    Q_INVOKABLE void setChecked(int row, int item, bool checked);
    Q_INVOKABLE void setTableCell(int row, int cell, const QString &text);
    Q_INVOKABLE void setImageUrl(int row, const QString &url);
    Q_INVOKABLE void setImageAlt(int row, const QString &alt);
    Q_INVOKABLE void appendTextBlock();
    Q_INVOKABLE void appendText(const QString &text);
    Q_INVOKABLE void appendImage(const QString &url, const QString &alt);
    Q_INVOKABLE void removeBlock(int row);

signals:
    void markdownChanged();
    void contentsChanged();

private:
    struct Block {
        BlockType    type = Text;
        QString      text;
        QStringList  items;
        QVariantList checked;
        QStringList  cells;
        int          columns = 0;
        QString      url;
        QString      alt;
    };

    static QList<Block> parseMarkdown(const QString &source);
    static QString      writeMarkdown(const QList<Block> &blocks);
    void                changed(int row, const QList<int> &roles);

    QList<Block> blocks_;
    bool         markdown_ = false;
};

} // namespace QtNote

#endif
