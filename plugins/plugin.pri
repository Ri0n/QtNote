include(../common.pri)
CONFIG += plugin
TEMPLATE = lib

unix {
	target.path = $$LIBDIR/$$APPNAME
	INSTALLS += target
}

HEADERS += $$PWD/trayiconinterface.h \
	$$PWD/qtnoteplugininterface.h
