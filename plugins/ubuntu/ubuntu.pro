include(../plugin.pri)

QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
}

SOURCES = \
    ubuntu.cpp \
    ubuntutray.cpp

HEADERS += \
    ubuntu.h \
    ubuntutray.h

#RESOURCES += \
#    main.qrc
