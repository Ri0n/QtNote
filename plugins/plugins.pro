include(../common.pri)

TEMPLATE = subdirs

SUBDIRS += baseintegration

exists(/usr/include/KDE):!nokde:SUBDIRS += kdeintegration
unix:!mac:!noubuntu:SUBDIRS += ubuntu
!notomboy:SUBDIRS += tomboy
!nospellcheker: SUBDIRS += spellchecker

#!nocrypt:SUBDIRS += crypt

HEADERS += qtnoteplugininterface.h \
	deintegrationinterface.h \
	trayinterface.h \
	globalshortcutsinterface.h \
	pluginoptionsinterface.h
