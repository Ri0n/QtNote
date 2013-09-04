include(../common.pri)
CONFIG += plugin
TEMPLATE = lib

unix {
	target.path = $$LIBDIR/$$APPNAME
	INSTALLS += target
}
