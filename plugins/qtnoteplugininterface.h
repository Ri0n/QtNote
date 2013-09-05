#ifndef QTNOTEPLUGININTERFACE_H
#define QTNOTEPLUGININTERFACE_H

#include <QVariantHash>
#include <QIcon>

struct PluginMetadata
{
	QString fileName;
	QString name;
	QString description;
	QString version;
	QString author;
	QIcon icon;
	QVariantHash extra;
};

class QtNotePluginInterface
{
public:
	virtual PluginMetadata metadata() = 0;
};

Q_DECLARE_INTERFACE(QtNotePluginInterface,
					 "com.rion-soft.QtNote.PluginInterface/1.0")

#endif // QTNOTEPLUGININTERFACE_H
