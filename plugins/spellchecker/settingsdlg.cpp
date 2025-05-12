#include <QLocale>
#include <QMessageBox>
#include <QPainter>
#include <QStyledItemDelegate>

#include "dictionarydownloader.h"
#include "engineinterface.h"
#include "settingsdlg.h"
#include "spellcheckplugin.h"
#include "ui_settingsdlg.h"

namespace QtNote {

#define TRASH_ICON ":/icons/trash"
#define DOWNLOAD_ICON ":/svg/download"

// NOTE it's a copypaste of a delegate from optionsplugins.cpp
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

class LocaleWidgetItem : public QTableWidgetItem {
public:
    SpellCheckPlugin::Dict dict;

    LocaleWidgetItem(const SpellCheckPlugin::Dict &dict) :
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QTableWidgetItem(dict.locale.nativeLanguageName() + QLatin1String(" (") + dict.locale.nativeTerritoryName()
#else
        QTableWidgetItem(dict.locale.nativeLanguageName() + QLatin1String(" (") + dict.locale.nativeCountryName()
#endif
                         + QLatin1Char(')')),
        dict(dict)
    {
    }
};

SettingsDlg::SettingsDlg(SpellCheckPlugin *plugin, QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDlg)
{
    ui->setupUi(this);

    ui->tblLangs->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tblLangs->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    ButtonDelegate *btnsDelegate = new ButtonDelegate(this);
    ui->tblLangs->setItemDelegateForColumn(1, btnsDelegate);

    dicts_ = plugin->dictionaries();
    auto hasUserPrefs
        = std::ranges::any_of(dicts_, [](auto const &d) { return d.flags & SpellCheckPlugin::DictUserSelected; });
    auto activeCheck = hasUserPrefs ? Qt::Checked : Qt::PartiallyChecked;

    ui->pbUseSystemPrefs->setEnabled(hasUserPrefs);

    ui->tblLangs->setRowCount(dicts_.count());
    for (int row = 0; row < dicts_.count(); row++) {
        auto             &dict = dicts_[row];
        LocaleWidgetItem *item = new LocaleWidgetItem(dict);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setCheckState((hasUserPrefs ? (dict.flags & SpellCheckPlugin::DictUserSelected)
                                          : (dict.flags & SpellCheckPlugin::DictSystemSelected))
                                ? activeCheck
                                : Qt::Unchecked);
        // item->font()
        ui->tblLangs->setItem(row, 0, item);

        auto icon = QIcon(dict.flags & SpellCheckPlugin::DictInstalled ? TRASH_ICON : DOWNLOAD_ICON);
        if (!icon.isNull()) {
            auto downloadBtn = new QTableWidgetItem();
            downloadBtn->setIcon(icon);
            ui->tblLangs->setItem(row, 1, downloadBtn);
        }
    }
    connect(ui->tblLangs, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
        if (item->column() != 0) {
            return; // checkbox is only on the first column
        }
        ui->tblLangs->blockSignals(true);
        for (int i = 0; i < ui->tblLangs->rowCount(); i++) {
            auto item = ui->tblLangs->item(i, 0);
            if (item->checkState() == Qt::PartiallyChecked) {
                item->setCheckState(Qt::Checked);
            }
        }
        ui->tblLangs->blockSignals(false);
        ui->pbUseSystemPrefs->setEnabled(true);
    });
    connect(ui->tblLangs, &QTableWidget::clicked, this, [plugin, this](const QModelIndex &index) {
        if (index.column() != 1) {
            return;
        }
        auto item = static_cast<LocaleWidgetItem *>(ui->tblLangs->item(index.row(), 0));
        if (item->dict.flags & SpellCheckPlugin::DictInstalled) {
            if (item->dict.flags & SpellCheckPlugin::DictWritable) {
                plugin->removeDictionary(item->dict.locale);
                ui->tblLangs->item(index.row(), 1)->setIcon(QIcon(DOWNLOAD_ICON));
                item->dict.flags &= ~SpellCheckPlugin::DictInstalled;
            }
        } else {
            auto downloader = plugin->download(item->dict.locale);
            connect(downloader, &DictionaryDownloader::finished, this, [item, downloader, this, row = index.row()]() {
                if (downloader->hasErrors()) {
                    QMessageBox::warning(this, tr("Download error"), downloader->lastError());
                } else {
                    ui->tblLangs->item(row, 1)->setIcon(QIcon(TRASH_ICON));
                    item->dict.flags |= SpellCheckPlugin::DictInstalled;
                }
            });
        }
    });
}

SettingsDlg::~SettingsDlg() { delete ui; }

QList<QLocale> SettingsDlg::preferredList() const
{
    QList<QLocale> ret;
    for (int i = 0; i < ui->tblLangs->rowCount(); i++) {
        auto item = (LocaleWidgetItem *)ui->tblLangs->item(i, 0);
        if (item->checkState() == Qt::Checked) {
            ret.append(item->dict.locale);
        }
    }
    return ret;
}

} // namespace QtNote

#include "settingsdlg.moc"
