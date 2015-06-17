include(../plugin.pri)

QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
	unix:!mac:QT *= widgets x11extras
}

SOURCES = \
    ubuntu.cpp \
    ubuntutray.cpp \
    x11util.cpp

HEADERS += \
    ubuntu.h \
    ubuntutray.h \
    x11util.h

RESOURCES += \
    ubuntu.qrc
