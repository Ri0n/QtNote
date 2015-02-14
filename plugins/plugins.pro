include(../common.pri)

TEMPLATE = subdirs

SUBDIRS += baseintegration \
    spellchecker
exists(/usr/include/KDE):!nokde:SUBDIRS += kdeintegration
!noubuntu:SUBDIRS += ubuntu
!notomboy:SUBDIRS += tomboy

#!nocrypt:SUBDIRS += crypt

HEADERS += qtnoteplugininterface.h \
	deintegrationinterface.h \
	trayinterface.h \
	globalshortcutsinterface.h \
	pluginoptionsinterface.h
