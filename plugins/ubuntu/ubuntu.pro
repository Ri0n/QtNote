include(../plugin.pri)

QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
}

SOURCES = \
    ubuntu.cpp \
    ubuntutray.cpp \
    statusnotifieritem.cpp

HEADERS += \
    ubuntu.h \
    ubuntutray.h \
    statusnotifieritem.h

RESOURCES += \
    main.qrc
