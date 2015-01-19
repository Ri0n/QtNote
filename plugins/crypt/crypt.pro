include(../plugin.pri)

QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
}

CONFIG += crypto

SOURCES = \
    notecrypt.cpp

HEADERS += \
    notecrypt.h
