#ifndef FILESTORAGESETTINGSWIDGET_H
#define FILESTORAGESETTINGSWIDGET_H

#include <QWidget>

namespace Ui {
class FileStorageSettingsWidget;
}

namespace QtNote {
class FileStorage;
}

class FileStorageSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit FileStorageSettingsWidget(const QString &customPath, QtNote::FileStorage *storage,
                                       QWidget *parent = nullptr);
    ~FileStorageSettingsWidget();
    QString path() const;

signals:
    void apply();

private slots:
    void on_pbBrowse_clicked();

    void on_cbCustomPath_clicked();

private:
    Ui::FileStorageSettingsWidget *ui;
    QtNote::FileStorage *          storage;
};

#endif // FILESTORAGESETTINGSWIDGET_H
