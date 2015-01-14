include(../plugin.pri)

LIBS += -lkdeui
QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
}

SOURCES = \
	kdeintegration.cpp \
    kdeintegrationtray.cpp

HEADERS += \
	kdeintegration.h \
    kdeintegrationtray.h

RESOURCES += \
    main.qrc
