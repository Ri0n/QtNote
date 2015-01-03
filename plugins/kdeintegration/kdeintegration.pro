include(../plugin.pri)

LIBS += -lkdeui
QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
}

SOURCES = \
	kdeintegration.cpp

HEADERS += \
	kdeintegration.h

RESOURCES += \
    main.qrc
