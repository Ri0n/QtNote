include(../common.pri)

TEMPLATE = subdirs

SUBDIRS += baseintegration

exists(/usr/include/KDE):!win32:!nokde:SUBDIRS += kdeintegration
unix:!mac:!noubuntu:SUBDIRS += ubuntu
!notomboy:SUBDIRS += tomboy

!nospellcheker {
	!win32|!isEmpty(HUNSPELL_DIR)|!isEmpty(HUNSPELL_LIB) {
		SUBDIRS += spellchecker
	} else {
		warning( Pass HUNSPELL_DIR or HUNSPELL_LIB and HUNSPELL_INC to qmake to build spellchecker plugin. )
	}
}

#!nocrypt:SUBDIRS += crypt

HEADERS += qtnoteplugininterface.h \
	deintegrationinterface.h \
	trayinterface.h \
	globalshortcutsinterface.h \
	pluginoptionsinterface.h
