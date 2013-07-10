#include "ptfstoragesettingswidget.h"
#include "ui_ptfstoragesettingswidget.h"

PTFStorageSettingsWidget::PTFStorageSettingsWidget(const QString &path, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PTFStorageSettingsWidget)
{
	ui->setupUi(this);
	ui->lePath->setText(path);
}

PTFStorageSettingsWidget::~PTFStorageSettingsWidget()
{
	delete ui;
}

QString PTFStorageSettingsWidget::path() const
{
	return ui->lePath->text();
}
