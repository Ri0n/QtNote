include(../plugin.pri)

QT *= gui
QT *= xml
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
}

SOURCES = \
	tomboydata.cpp \
	tomboyplugin.cpp \
	tomboystorage.cpp

HEADERS += \
	tomboydata.h \
	tomboyplugin.h \
	tomboystorage.h

RESOURCES += \
    tomboy.qrc
