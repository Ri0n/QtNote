#include <QAbstractTableModel>

#include "optionsplugins.h"
#include "ui_optionsplugins.h"
#include "qtnote.h"
#include "pluginmanager.h"

namespace QtNote {

class PluginsModel : public QAbstractTableModel
{
	Main *qtnote;
	QStringList pluginNames; // by priority

public:
	PluginsModel(Main *qtnote, QObject *parent) :
		QAbstractTableModel(parent),
		qtnote(qtnote)
	{
		pluginNames = qtnote->pluginManager()->pluginsNames();
	}

	int rowCount(const QModelIndex &parent = QModelIndex()) const
	{
		if (parent.isValid()) {
			return 0;
		}
		return pluginNames.count();
	}

	int columnCount(const QModelIndex &parent = QModelIndex()) const
	{
		if (parent.isValid()) {
			return 0;
		}
		return 1;
	}

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
	{
		if (index.column() == 0) {
			if (role == Qt::CheckStateRole)
			{
				PluginManager::LoadPolicy lp = qtnote->pluginManager()->loadPolicy(pluginNames[index.row()]);
				switch (lp)
				{
				case PluginManager::LP_Auto:
					return Qt::PartiallyChecked;
				case PluginManager::LP_Disabled:
					return Qt::Unchecked;
				case PluginManager::LP_Enabled:
					return Qt::Checked;
				}
			} else if (role == Qt::DisplayRole) {
				return pluginNames[index.row()];
			} else if (role == Qt::DecorationRole) {
				return qtnote->pluginManager()->icon(pluginNames[index.row()]);
			}
		}
		return QVariant();
	}

	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
	{
		if (index.column() == 0 && role == Qt::CheckStateRole) {
			Qt::CheckState cs = (Qt::CheckState)value.value<int>();
			if (data(index, role) == Qt::Unchecked) {
				cs = Qt::PartiallyChecked;
			}
			qtnote->pluginManager()->setLoadPolicy(pluginNames[index.row()],
						cs == Qt::PartiallyChecked? PluginManager::LP_Auto :
						(cs == Qt::Checked? PluginManager::LP_Enabled : PluginManager::LP_Disabled));
			emit dataChanged(index, index);
			return true;
		}
		return false;
	}

	Qt::ItemFlags flags(const QModelIndex &index) const
	{
		if (index.column() == 0) {
			return QAbstractTableModel::flags(index) | Qt::ItemIsTristate | Qt::ItemIsUserCheckable;
		}
		return QAbstractTableModel::flags(index);
	}
};

class MouseDisabler : public QObject
{
public:
	MouseDisabler(QObject  *parent) : QObject(parent) {}
	bool eventFilter(QObject *obj, QEvent *event)
	 {
		 if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
			 return true;
		 }
		 return QObject::eventFilter(obj, event);
	 }
};

OptionsPlugins::OptionsPlugins(Main *qtnote, QWidget *parent) :
    QWidget(parent),
	ui(new Ui::OptionsPlugins),
	qtnote(qtnote)
{
	ui->setupUi(this);

	MouseDisabler *md = new MouseDisabler(this);
	ui->ckLegendAuto->installEventFilter(md);
	ui->ckLegendEnabled->installEventFilter(md);
	ui->ckLegendDisabled->installEventFilter(md);
	ui->ckLegendAuto->setCheckState(Qt::PartiallyChecked);

	pluginsModel = new PluginsModel(qtnote, this);
	ui->tblPlugins->setModel(pluginsModel);
#if QT_VERSION >= 0x050000
	ui->tblPlugins->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
#else
	ui->tblPlugins->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
#endif
}

OptionsPlugins::~OptionsPlugins()
{
	delete ui;
}

} // namespace QtNote
