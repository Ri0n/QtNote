#ifndef QTNOTEPLUGININTERFACE_H
#define QTNOTEPLUGININTERFACE_H

#include <QVariantHash>
#include <QIcon>

#include "qtnote.h"

namespace QtNote {

struct PluginMetadata
{
	enum PluginType {
		DEIntegration,
		NoteStorage,
		Other
	};
	PluginType pluginType;
	QIcon icon;
	QString name;
	QString description;
	QString author;
	quint32 version;	// plugin's version 0xXXYYZZPP
	quint32 minVersion; // minimum compatible version of QtNote
	quint32 maxVersion; // maximum compatible version of QtNote
	QVariantHash extra;
};

class PluginInterface
{
public:
	virtual PluginMetadata metadata() = 0;
	virtual bool init(Main *qtnote) = 0;
};

class PluginOptionsTooltipInterface
{
public:
	virtual QString tooltip() const = 0;
};


} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::PluginInterface,
					 "com.rion-soft.QtNote.PluginInterface/1.0")

Q_DECLARE_INTERFACE(QtNote::PluginOptionsTooltipInterface,
					 "com.rion-soft.QtNote.PluginOptionsTooltipInterface/1.0")

#endif // QTNOTEPLUGININTERFACE_H
