#include <QFileDialog>

#include "filestoragesettingswidget.h"
#include "ui_filestoragesettingswidget.h"
#include "filestorage.h"

FileStorageSettingsWidget::FileStorageSettingsWidget(const QString &customPath, QtNote::FileStorage *storage, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileStorageSettingsWidget),
    storage(storage)
{
    ui->setupUi(this);
    ui->cbCustomPath->setChecked(!customPath.isEmpty());
    ui->leStoragePath->setText(customPath);
    on_cbCustomPath_clicked();
    ui->leStoragePath->setCursorPosition(0);
}

FileStorageSettingsWidget::~FileStorageSettingsWidget()
{
    delete ui;
}

QString FileStorageSettingsWidget::path() const
{
    return ui->leStoragePath->text();
}

void FileStorageSettingsWidget::on_pbBrowse_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose storage directory"), ui->leStoragePath->text());
    if (!dir.isEmpty()) {
        ui->leStoragePath->setText(dir);
    }
}

void FileStorageSettingsWidget::on_cbCustomPath_clicked()
{
    ui->leStoragePath->setEnabled(ui->cbCustomPath->isChecked());
    ui->pbBrowse->setEnabled(ui->cbCustomPath->isChecked());
    if (!ui->cbCustomPath->isChecked()) {
        ui->leStoragePath->setText(storage->findStorageDir());
    }
}
