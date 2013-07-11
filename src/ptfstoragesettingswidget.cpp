#include <QFileDialog>

#include "ptfstoragesettingswidget.h"
#include "ui_ptfstoragesettingswidget.h"

PTFStorageSettingsWidget::PTFStorageSettingsWidget(const QString &path, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PTFStorageSettingsWidget)
{
	ui->setupUi(this);
	ui->lePath->setText(path);
	ui->lePath->setCursorPosition(0);
}

PTFStorageSettingsWidget::~PTFStorageSettingsWidget()
{
	delete ui;
}

QString PTFStorageSettingsWidget::path() const
{
	return ui->lePath->text();
}

void PTFStorageSettingsWidget::on_btnBrowse_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Choose storage directory"), ui->lePath->text());
	if (!dir.isEmpty()) {
		ui->lePath->setText(dir);
	}
}
