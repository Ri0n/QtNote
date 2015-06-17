include(../common.pri)
CONFIG += plugin
TEMPLATE = lib

win32 {
	CONFIG += skip_target_version_ext
	LIBS += -L$$OUT_PWD/../../libqtnote$$DBG_SUBDIR -lqtnote

	!isEmpty(WININST_PREFIX) {
		target.path = $$WININST_PREFIX/plugins
		INSTALLS += target
	}
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
