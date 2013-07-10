#ifndef PTFSTORAGESETTINGSWIDGET_H
#define PTFSTORAGESETTINGSWIDGET_H

#include <QWidget>

namespace Ui {
class PTFStorageSettingsWidget;
}

class PTFStorageSettingsWidget : public QWidget
{
	Q_OBJECT
	
public:
	explicit PTFStorageSettingsWidget(const QString &path, QWidget *parent = 0);
	~PTFStorageSettingsWidget();
	QString path() const;

signals:
	void apply();
	
private:
	Ui::PTFStorageSettingsWidget *ui;
};

#endif // PTFSTORAGESETTINGSWIDGET_H
