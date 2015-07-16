include(../common.pri)
CONFIG += plugin
TEMPLATE = lib

win32 {
	CONFIG += skip_target_version_ext
	LIBS += -L$$OUT_PWD/../../libqtnote$$DBG_SUBDIR -lqtnote

	target.path = $$WININST_PREFIX/plugins
	INSTALLS += target
	DEFINES += QTNOTE_IMPORT
}

unix {
	target.path = $$LIBDIR/$$APPNAME
	INSTALLS += target
}

HEADERS += $$PWD/deintegrationinterface.h \
	$$PWD/qtnoteplugininterface.h

INCLUDEPATH *= $$PWD $$PWD/../libqtnote
DEPENDPATH *= $$PWD $$PWD/../libqtnote

DEFINES += QTNOTE_PLUGIN
