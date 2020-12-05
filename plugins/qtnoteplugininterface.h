#ifndef QTNOTEPLUGININTERFACE_H
#define QTNOTEPLUGININTERFACE_H

#include <QIcon>
#include <QUrl>
#include <QVariantHash>

#include "pluginhostinterface.h"
#include "qtnote.h"

namespace QtNote {

#define MetadataVerion 2

struct PluginMetadata {
    QIcon        icon;
    QString      id;
    QString      name;
    QString      description;
    QString      author;
    quint32      version;    // plugin's version 0xXXYYZZPP
    quint32      minVersion; // minimum compatible version of QtNote
    quint32      maxVersion; // maximum compatible version of QtNote
    QUrl         homepage;
    QVariantHash extra;
};

class PluginInterface {
public:
    virtual int            metadataVersion() const            = 0;
    virtual PluginMetadata metadata()                         = 0;
    virtual void           setHost(PluginHostInterface *host) = 0;
};

class RegularPluginInterface {
public:
    virtual bool init(Main *qtnote) = 0;
};

class PluginOptionsTooltipInterface {
public:
    virtual QString tooltip() const = 0;
};

} // namespace QtNote

Q_DECLARE_INTERFACE(QtNote::PluginInterface, "com.rion-soft.QtNote.PluginInterface/2.0")

Q_DECLARE_INTERFACE(QtNote::RegularPluginInterface, "com.rion-soft.QtNote.RegularPluginInterface/1.0")

Q_DECLARE_INTERFACE(QtNote::PluginOptionsTooltipInterface, "com.rion-soft.QtNote.PluginOptionsTooltipInterface/1.0")

#endif // QTNOTEPLUGININTERFACE_H
