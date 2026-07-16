#include <QAbstractTableModel>
#include <QDataStream>
#include <QDebug>
#include <QDialog>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTransform>

#include "optionsplugins.h"
#include "pluginmanager.h"
#include "qtnote.h"
#include "ui_optionsplugins.h"

namespace QtNote {

class ButtonDelegate : public QStyledItemDelegate {
    Q_OBJECT

    QModelIndex sunken;

public:
    explicit ButtonDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) { }

    // painting
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        if (opt.icon.isNull()) {
            return;
        }
        painter->save();
        if (opt.state & QStyle::State_Selected) {
            painter->setPen(QPen(Qt::NoPen));
            if (opt.state & QStyle::State_Active) {
                painter->setBrush(QBrush(QPalette().highlight()));
            } else {
                painter->setBrush(QBrush(QPalette().color(QPalette::Inactive, QPalette::Highlight)));
            }
            painter->drawRect(opt.rect);
        }

        QStyleOptionButton buttonOption;
        buttonOption.icon     = opt.icon;
        buttonOption.iconSize = option.decorationSize;
        buttonOption.text     = opt.text;
        buttonOption.features = QStyleOptionButton::Flat;
        buttonOption.rect     = opt.rect;
        buttonOption.state    = QStyle::State_Enabled;
        if (index == sunken) {
            buttonOption.state |= QStyle::State_Sunken;
        }
        if (option.state & QStyle::State_MouseOver) {
            buttonOption.state |= (QStyle::State_Active | QStyle::State_MouseOver);
        }

        QApplication::style()->drawControl(QStyle::CE_PushButton, &buttonOption, painter);
        painter->restore();
    }

    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index)
    {
        Q_UNUSED(model);
        Q_UNUSED(option);

        if (!(event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)) {
            return true;
        }

        sunken = QModelIndex();

        if (event->type() == QEvent::MouseButtonPress) {
            sunken = index;
        }
        return true;
    }
};

class PluginsModel : public QAbstractTableModel {
    Q_OBJECT

    static constexpr auto MimeType = "application/qtnote.plugin.id";

    Main       *qtnote;
    QStringList pluginIds; // by priority
    QIcon       settingIcon;

public:
    PluginsModel(Main *qtnote, QObject *parent) : QAbstractTableModel(parent), qtnote(qtnote)
    {
        pluginIds = qtnote->pluginManager()->pluginsIds();
        QPixmap pix(":/icons/options");
        settingIcon = QIcon(pix);
        QTransform transform;
        transform.rotate(45);
        pix.transformed(transform, Qt::SmoothTransformation);

        settingIcon.addPixmap(pix.transformed(transform), QIcon::Active);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid()) {
            return 0;
        }
        return pluginIds.count();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid()) {
            return 0;
        }
        return 3;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        QString pluginId = pluginIds[index.row()];
        if (index.column() == 0) {
            switch (role) {
            case Qt::CheckStateRole: {
                PluginManager::LoadPolicy lp = qtnote->pluginManager()->loadPolicy(pluginId);
                switch (lp) {
                case PluginManager::LP_Auto:
                    return Qt::PartiallyChecked;
                case PluginManager::LP_Disabled:
                    return Qt::Unchecked;
                case PluginManager::LP_Enabled:
                    return Qt::Checked;
                }
                break;
            }

            case Qt::DisplayRole:
                return qtnote->pluginManager()->metadata(pluginId).name;

            case Qt::DecorationRole: {
                QIcon icon = qtnote->pluginManager()->metadata(pluginId).icon;
                return icon.isNull() ? QIcon(":/icons/plugin") : icon;
            }

            case Qt::ToolTipRole: {
                PluginManager::LoadStatus                       status = qtnote->pluginManager()->loadStatus(pluginId);
                static QMap<PluginManager::LoadStatus, QString> strStatus;
                if (strStatus.isEmpty()) {
                    strStatus.insert(PluginManager::LS_ErrAbi, tr("ABI mismatch"));
                    strStatus.insert(PluginManager::LS_ErrMetadata, tr("Incompatible metadata"));
                    strStatus.insert(PluginManager::LS_ErrVersion, tr("Incompatible version"));
                    strStatus.insert(PluginManager::LS_Loaded, tr("Loaded"));
                    strStatus.insert(PluginManager::LS_Initialized, tr("Initialized"));
                    strStatus.insert(PluginManager::LS_ErrNotPlugin, tr("Not a plugin"));
                    strStatus.insert(PluginManager::LS_Undefined, tr("Not loaded"));
                    strStatus.insert(PluginManager::LS_Unloaded, tr("Not loaded"));
                }
                QString ret = qtnote->pluginManager()->metadata(pluginId).description + QLatin1String("<br/><br/>")
                    + tr("<b>Filename:</b> %1").arg(qtnote->pluginManager()->filename(pluginId)) + "<br/><br/>"
                    + tr("<b>Status:</b> %1").arg(strStatus[status]);
                if (status && status < PluginManager::LS_Errors) {
                    QString tooltip = qtnote->pluginManager()->tooltip(pluginId);
                    if (!tooltip.isEmpty()) {
                        ret += QLatin1String("<br/><br/>");
                        ret += tooltip;
                    }
                }
                return ret;
            }

            case Qt::FontRole: {
                QFont                     f; // application default font. may by not what we expect
                PluginManager::LoadStatus status = qtnote->pluginManager()->loadStatus(pluginId);
                if (status == PluginManager::LS_Initialized) {
                    f.setBold(true);
                }
                return f;
            }

            case Qt::ForegroundRole: {
                QColor color = qApp->palette().color(QPalette::WindowText); // mey be not what we expect
                PluginManager::LoadStatus status = qtnote->pluginManager()->loadStatus(pluginId);
                if (!(status == PluginManager::LS_Initialized || status == PluginManager::LS_Initialized)) {
                    color.setAlpha(128);
                }
                return color;
            }
            }
        } else if (index.column() == 1) { // version
            if (role == Qt::DisplayRole) {
                auto        version = qtnote->pluginManager()->metadata(pluginId).version;
                QStringList ret;
                while (version) {
                    ret.append(QString::number((version & 0xff000000) >> 24));
                    version <<= 8;
                }
                if (ret.count() < 2) {
                    ret.append("0");
                }
                return ret.join(".");
            }
        } else if (index.column() == 2) { // settings button
            // options button
            if (role == Qt::DecorationRole && qtnote->pluginManager()->canOptionsDialog(pluginId)) {
                return settingIcon;
            }
        }
        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (index.column() == 0 && role == Qt::CheckStateRole) {
            Qt::CheckState cs = (Qt::CheckState)value.value<int>();
            if (data(index, role) == Qt::Unchecked) {
                cs = Qt::PartiallyChecked;
            }
            qtnote->pluginManager()->setLoadPolicy(
                pluginIds[index.row()],
                cs == Qt::PartiallyChecked
                    ? PluginManager::LP_Auto
                    : (cs == Qt::Checked ? PluginManager::LP_Enabled : PluginManager::LP_Disabled));
            emit dataChanged(index, index);
            return true;
        }
        return false;
    }

    QStringList mimeTypes() const override { return { QLatin1String(MimeType) }; }

    QMimeData *mimeData(const QModelIndexList &indexes) const override
    {
        if (indexes.isEmpty()) {
            return nullptr;
        }

        const auto row = indexes.first().row();
        if (row < 0 || row >= pluginIds.size()) {
            return nullptr;
        }

        auto        mimeData = new QMimeData();
        QByteArray  encodedData;
        QDataStream out(&encodedData, QIODevice::WriteOnly);
        out << pluginIds.at(row);
        mimeData->setData(QLatin1String(MimeType), encodedData);
        return mimeData;
    }

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                      const QModelIndex &parent) override
    {
        Q_UNUSED(column);

        if (action == Qt::IgnoreAction) {
            return true;
        }
        if (action != Qt::MoveAction || !data->hasFormat(QLatin1String(MimeType))) {
            return false;
        }

        QString     pluginId;
        QByteArray  encodedData = data->data(QLatin1String(MimeType));
        QDataStream in(&encodedData, QIODevice::ReadOnly);
        in >> pluginId;

        const auto sourceRow = pluginIds.indexOf(pluginId);
        if (sourceRow < 0) {
            return false;
        }

        int destinationRow = row;
        if (destinationRow < 0) {
            destinationRow = parent.isValid() ? parent.row() : pluginIds.size();
        }
        return moveRows(QModelIndex(), sourceRow, 1, QModelIndex(), destinationRow);
    }

    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent,
                  int destinationChild) override
    {
        if (sourceParent.isValid() || destinationParent.isValid() || count != 1 || sourceRow < 0
            || sourceRow >= pluginIds.size() || destinationChild < 0 || destinationChild > pluginIds.size()
            || destinationChild == sourceRow || destinationChild == sourceRow + 1) {
            return false;
        }

        beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, destinationChild);
        pluginIds.move(sourceRow, destinationChild > sourceRow ? destinationChild - 1 : destinationChild);
        endMoveRows();

        QSettings().setValue(QLatin1String("plugins-priority"), pluginIds);
        return true;
    }

    Qt::DropActions supportedDragActions() const override { return Qt::MoveAction; }
    Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (!index.isValid()) {
            return QAbstractTableModel::flags(index) | Qt::ItemIsDropEnabled;
        }

        auto flags = QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        if (index.column() == 0) {
            flags |= Qt::ItemIsUserTristate | Qt::ItemIsUserCheckable;
        }
        return flags;
    }

    QString pluginId(int row) const { return pluginIds.at(row); }
};

class MouseDisabler : public QObject {
public:
    MouseDisabler(QObject *parent) : QObject(parent) { }
    bool eventFilter(QObject *obj, QEvent *event)
    {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
            return true;
        }
        return QObject::eventFilter(obj, event);
    }
};

OptionsPlugins::OptionsPlugins(Main *qtnote, QWidget *parent) :
    QWidget(parent), ui(new Ui::OptionsPlugins), qtnote(qtnote)
{
    ui->setupUi(this);

    MouseDisabler *md = new MouseDisabler(this);
    ui->ckLegendAuto->installEventFilter(md);
    ui->ckLegendEnabled->installEventFilter(md);
    ui->ckLegendDisabled->installEventFilter(md);
    ui->ckLegendAuto->setCheckState(Qt::PartiallyChecked);

    pluginsModel = new PluginsModel(qtnote, this);
    ui->tblPlugins->setModel(pluginsModel);
    ui->tblPlugins->setDragDropMode(QAbstractItemView::InternalMove);
    ui->tblPlugins->setDefaultDropAction(Qt::MoveAction);
    ui->tblPlugins->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tblPlugins->setSelectionMode(QAbstractItemView::SingleSelection);
    ButtonDelegate *btnsDelegate = new ButtonDelegate();
    ui->tblPlugins->setItemDelegateForColumn(2, btnsDelegate);
    ui->tblPlugins->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tblPlugins->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tblPlugins->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    connect(ui->tblPlugins, SIGNAL(clicked(QModelIndex)), SLOT(pluginClicked(QModelIndex)));
}

OptionsPlugins::~OptionsPlugins() { delete ui; }

void OptionsPlugins::pluginClicked(const QModelIndex &index)
{
    if (index.column() == 2) { // settings
        QString  id = pluginsModel->pluginId(index.row());
        QDialog *d  = qtnote->pluginManager()->optionsDialog(id);
        if (d) {
            auto &md = qtnote->pluginManager()->metadata(id);
            d->setWindowTitle(md.name + QStringLiteral(": ") + tr("Settings"));
            d->setWindowIcon(md.icon);
            connect(d, &QDialog::accepted, this, [this, id]() {
                qDebug() << "Plugin settings accepted:" << id << "emitting settingsUpdated";
                emit qtnote->settingsUpdated();
            });
            d->show();
            d->raise();
        }
    }
}

} // namespace QtNote

#include "optionsplugins.moc"
