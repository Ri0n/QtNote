include(../plugin.pri)
include(3rdparty/qxt/qxt.pri)

LIBS += -lkdeui
QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
}

SOURCES = \
	baseintegration.cpp

HEADERS += \
	baseintegration.h

RESOURCES += \
    base.qrc
