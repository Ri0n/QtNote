include(../common.pri)

TEMPLATE = subdirs

SUBDIRS += baseintegration
exists(/usr/include/KDE):!nokde:SUBDIRS += kdeintegration
ubuntu:SUBDIRS += ubuntu

#!nocrypt:SUBDIRS += crypt

HEADERS += qtnoteplugininterface.h \
	deintegrationinterface.h \
	trayinterface.h \
	globalshortcutsinterface.h
