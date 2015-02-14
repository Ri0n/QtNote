#ifndef PLUGINOPTIONSINTERFACE_H
#define PLUGINOPTIONSINTERFACE_H

class QDialog;

namespace QtNote {

class PluginOptionsInterface
{
public:
	virtual QDialog* optionsDialog() = 0;
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::PluginOptionsInterface,
					 "com.rion-soft.QtNote.PluginOptionsInterface/1.0")

#endif
