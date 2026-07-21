#ifndef PLUGINLISTMODEL_H
#define PLUGINLISTMODEL_H

#include "pluginmanager.h"
#include "qtnote_export.h"

#include <QAbstractListModel>

namespace QtNote {

class QTNOTE_EXPORT PluginListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        PluginIdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        VersionTextRole,
        LoadPolicyRole,
        LoadStatusRole,
        LoadedRole,
        EnabledRole,
        ConfigurableRole,
        TooltipRole,
        IconRole,
    };
    Q_ENUM(Role)

    explicit PluginListModel(PluginManager *pluginManager, QObject *parent = nullptr);

    int                    rowCount(const QModelIndex &parent = {}) const override;
    QVariant               data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool                   setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags          flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;
    QStringList            mimeTypes() const override;
    QMimeData             *mimeData(const QModelIndexList &indexes) const override;
    bool                   dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                                        const QModelIndex &parent) override;
    Qt::DropActions        supportedDragActions() const override;
    Qt::DropActions        supportedDropActions() const override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent,
                  int destinationChild) override;

    QString pluginId(int row) const;
    bool    setLoadPolicy(int row, PluginManager::LoadPolicy loadPolicy);

    Q_INVOKABLE bool setLoadPolicy(int row, int loadPolicy);
    Q_INVOKABLE bool setEnabled(int row, bool enabled);

private:
    QString versionText(const QString &pluginId) const;
    QString tooltipText(const QString &pluginId) const;

    PluginManager *pluginManager_ = nullptr;
    QStringList    pluginIds_;
};

} // namespace QtNote

#endif // PLUGINLISTMODEL_H
