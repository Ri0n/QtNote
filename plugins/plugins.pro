include(../common.pri)

TEMPLATE = subdirs

SUBDIRS += baseintegration
exists(/usr/include/KDE):!nokde:SUBDIRS += kdeintegration

#!nocrypt:SUBDIRS += crypt

HEADERS += qtnoteplugininterface.h \
	deintegrationinterface.h \
	trayinterface.h \
	globalshortcutsinterface.h
