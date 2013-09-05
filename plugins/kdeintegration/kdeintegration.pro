include(../plugin.pri)

LIBS += -lkdeui
QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
}

SOURCES = \
	trayicon.cpp

HEADERS += \
	trayicon.h
