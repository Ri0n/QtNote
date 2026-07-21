#include "pluginlistmodel.h"

#include <QDataStream>
#include <QFont>
#include <QGuiApplication>
#include <QIODevice>
#include <QMimeData>
#include <QPalette>
#include <QSettings>

namespace QtNote {

namespace {
    constexpr auto MimeType = "application/qtnote.plugin.id";

    Qt::CheckState checkStateForPolicy(PluginManager::LoadPolicy loadPolicy)
    {
        switch (loadPolicy) {
        case PluginManager::LP_Auto:
            return Qt::PartiallyChecked;
        case PluginManager::LP_Disabled:
            return Qt::Unchecked;
        case PluginManager::LP_Enabled:
            return Qt::Checked;
        }
        return Qt::Unchecked;
    }

    QString statusText(PluginManager::LoadStatus status)
    {
        switch (status) {
        case PluginManager::LS_ErrAbi:
            return PluginListModel::tr("ABI mismatch");
        case PluginManager::LS_ErrMetadata:
            return PluginListModel::tr("Incompatible metadata");
        case PluginManager::LS_ErrVersion:
            return PluginListModel::tr("Incompatible version");
        case PluginManager::LS_Loaded:
            return PluginListModel::tr("Loaded");
        case PluginManager::LS_Initialized:
            return PluginListModel::tr("Initialized");
        case PluginManager::LS_ErrNotPlugin:
            return PluginListModel::tr("Not a plugin");
        case PluginManager::LS_Undefined:
        case PluginManager::LS_Unloaded:
            return PluginListModel::tr("Not loaded");
        }
        return PluginListModel::tr("Unknown");
    }
}

PluginListModel::PluginListModel(PluginManager *pluginManager, QObject *parent) :
    QAbstractListModel(parent), pluginManager_(pluginManager)
{
    if (pluginManager_)
        pluginIds_ = pluginManager_->pluginsIds();
}

int PluginListModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : pluginIds_.count(); }

QVariant PluginListModel::data(const QModelIndex &index, int role) const
{
    if (!pluginManager_ || !index.isValid() || index.row() < 0 || index.row() >= pluginIds_.size())
        return {};

    const QString pluginId = pluginIds_.at(index.row());
    const auto   &metadata = pluginManager_->metadata(pluginId);
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return metadata.name;
    case Qt::DecorationRole:
    case IconRole:
        return metadata.icon.isNull() ? QIcon(":/icons/plugin") : metadata.icon;
    case Qt::CheckStateRole:
        return checkStateForPolicy(pluginManager_->loadPolicy(pluginId));
    case Qt::ToolTipRole:
    case TooltipRole:
        return tooltipText(pluginId);
    case Qt::FontRole: {
        QFont font;
        if (pluginManager_->loadStatus(pluginId) == PluginManager::LS_Initialized)
            font.setBold(true);
        return font;
    }
    case Qt::ForegroundRole: {
        QColor color = qGuiApp ? qGuiApp->palette().color(QPalette::WindowText) : QColor(Qt::black);
        if (pluginManager_->loadStatus(pluginId) != PluginManager::LS_Initialized)
            color.setAlpha(128);
        return color;
    }
    case PluginIdRole:
        return pluginId;
    case DescriptionRole:
        return metadata.description;
    case VersionTextRole:
        return versionText(pluginId);
    case LoadPolicyRole:
        return int(pluginManager_->loadPolicy(pluginId));
    case LoadStatusRole:
        return int(pluginManager_->loadStatus(pluginId));
    case LoadedRole:
        return pluginManager_->isLoaded(pluginId);
    case EnabledRole:
        return pluginManager_->loadPolicy(pluginId) != PluginManager::LP_Disabled;
    case ConfigurableRole:
        return pluginManager_->canOptionsDialog(pluginId);
    default:
        return {};
    }
}

bool PluginListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!pluginManager_ || !index.isValid() || index.row() < 0 || index.row() >= pluginIds_.size())
        return false;

    if (role == Qt::CheckStateRole) {
        Qt::CheckState checkState = Qt::CheckState(value.toInt());
        if (data(index, role) == Qt::Unchecked)
            checkState = Qt::PartiallyChecked;
        return setLoadPolicy(
            index.row(),
            checkState == Qt::PartiallyChecked
                ? PluginManager::LP_Auto
                : (checkState == Qt::Checked ? PluginManager::LP_Enabled : PluginManager::LP_Disabled));
    }
    if (role == LoadPolicyRole)
        return setLoadPolicy(index.row(), value.toInt());
    if (role == EnabledRole)
        return setEnabled(index.row(), value.toBool());
    return false;
}

Qt::ItemFlags PluginListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return QAbstractListModel::flags(index) | Qt::ItemIsDropEnabled;

    return QAbstractListModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsUserTristate
        | Qt::ItemIsUserCheckable;
}

QHash<int, QByteArray> PluginListModel::roleNames() const
{
    return {
        { PluginIdRole, "pluginId" },
        { NameRole, "name" },
        { DescriptionRole, "description" },
        { VersionTextRole, "versionText" },
        { LoadPolicyRole, "loadPolicy" },
        { LoadStatusRole, "loadStatus" },
        { LoadedRole, "loaded" },
        { EnabledRole, "enabled" },
        { ConfigurableRole, "configurable" },
        { TooltipRole, "tooltip" },
        { IconRole, "icon" },
    };
}

QStringList PluginListModel::mimeTypes() const { return { QLatin1String(MimeType) }; }

QMimeData *PluginListModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.isEmpty())
        return nullptr;

    const auto row = indexes.first().row();
    if (row < 0 || row >= pluginIds_.size())
        return nullptr;

    auto       *mimeData = new QMimeData();
    QByteArray  encodedData;
    QDataStream out(&encodedData, QIODevice::WriteOnly);
    out << pluginIds_.at(row);
    mimeData->setData(QLatin1String(MimeType), encodedData);
    return mimeData;
}

bool PluginListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                                   const QModelIndex &parent)
{
    Q_UNUSED(column)

    if (action == Qt::IgnoreAction)
        return true;
    if (action != Qt::MoveAction || !data->hasFormat(QLatin1String(MimeType)))
        return false;

    QString     pluginId;
    QByteArray  encodedData = data->data(QLatin1String(MimeType));
    QDataStream in(&encodedData, QIODevice::ReadOnly);
    in >> pluginId;

    const auto sourceRow = pluginIds_.indexOf(pluginId);
    if (sourceRow < 0)
        return false;

    int destinationRow = row;
    if (destinationRow < 0)
        destinationRow = parent.isValid() ? parent.row() : pluginIds_.size();
    return moveRows({}, sourceRow, 1, {}, destinationRow);
}

Qt::DropActions PluginListModel::supportedDragActions() const { return Qt::MoveAction; }

Qt::DropActions PluginListModel::supportedDropActions() const { return Qt::MoveAction; }

bool PluginListModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                               const QModelIndex &destinationParent, int destinationChild)
{
    if (sourceParent.isValid() || destinationParent.isValid() || count != 1 || sourceRow < 0
        || sourceRow >= pluginIds_.size() || destinationChild < 0 || destinationChild > pluginIds_.size()
        || destinationChild == sourceRow || destinationChild == sourceRow + 1) {
        return false;
    }

    beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, destinationChild);
    pluginIds_.move(sourceRow, destinationChild > sourceRow ? destinationChild - 1 : destinationChild);
    endMoveRows();

    QSettings().setValue(QLatin1String("plugins-priority"), pluginIds_);
    return true;
}

QString PluginListModel::pluginId(int row) const
{
    return row >= 0 && row < pluginIds_.size() ? pluginIds_.at(row) : QString();
}

bool PluginListModel::setLoadPolicy(int row, PluginManager::LoadPolicy loadPolicy)
{
    if (!pluginManager_ || row < 0 || row >= pluginIds_.size())
        return false;

    const auto index = this->index(row);
    pluginManager_->setLoadPolicy(pluginIds_.at(row), loadPolicy);
    emit dataChanged(
        index, index,
        { Qt::CheckStateRole, Qt::FontRole, Qt::ForegroundRole, LoadPolicyRole, LoadedRole, EnabledRole, TooltipRole });
    return true;
}

bool PluginListModel::setLoadPolicy(int row, int loadPolicy)
{
    if (loadPolicy < PluginManager::LP_Auto || loadPolicy > PluginManager::LP_Disabled)
        return false;
    return setLoadPolicy(row, PluginManager::LoadPolicy(loadPolicy));
}

bool PluginListModel::setEnabled(int row, bool enabled)
{
    return setLoadPolicy(row, enabled ? PluginManager::LP_Auto : PluginManager::LP_Disabled);
}

QString PluginListModel::versionText(const QString &pluginId) const
{
    auto        version = pluginManager_->metadata(pluginId).version;
    QStringList parts;
    while (version) {
        parts.append(QString::number((version & 0xff000000) >> 24));
        version <<= 8;
    }
    if (parts.count() < 2)
        parts.append(QStringLiteral("0"));
    return parts.join(QLatin1Char('.'));
}

QString PluginListModel::tooltipText(const QString &pluginId) const
{
    const auto &metadata = pluginManager_->metadata(pluginId);
    QString     result   = metadata.description + QLatin1String("<br/><br/>")
        + tr("<b>Filename:</b> %1").arg(pluginManager_->filename(pluginId)) + QLatin1String("<br/><br/>")
        + tr("<b>Status:</b> %1").arg(statusText(pluginManager_->loadStatus(pluginId)));

    if (pluginManager_->isLoaded(pluginId)) {
        const QString tooltip = pluginManager_->tooltip(pluginId);
        if (!tooltip.isEmpty())
            result += QLatin1String("<br/><br/>") + tooltip;
    }
    return result;
}

} // namespace QtNote
