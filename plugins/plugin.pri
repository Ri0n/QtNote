include(../common.pri)
CONFIG += plugin
TEMPLATE = lib

unix {
	target.path = $$LIBDIR/$$APPNAME
	INSTALLS += target
}

HEADERS += $$PWD/deintegrationinterface.h \
	$$PWD/qtnoteplugininterface.h

INCLUDEPATH *= $$PWD $$PWD/../src

DEFINES += QTNOTE_PLUGIN
