include(../plugin.pri)
include(3rdparty/qxt/qxt.pri)

QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
    unix:!mac:QT *= x11extras
}

SOURCES += \
    baseintegration.cpp \
    baseintegrationtray.cpp

HEADERS += \
    baseintegration.h \
    baseintegrationtray.h

RESOURCES += \
    baseintegration.qrc

win32-ms*:LIBS += -lUser32
