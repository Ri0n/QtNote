include(../plugin.pri)

QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
	QT *= widgets KGlobalAccel KWindowSystem KNotifications
	DEFINES += USE_KDE5
} else {
	INCLUDEPATH += /usr/include/KDE
	LIBS += -lkdeui
}

SOURCES += \
	kdeintegration.cpp \
	kdeintegrationtray.cpp

HEADERS += \
	kdeintegration.h \
	kdeintegrationtray.h

RESOURCES += \
    kdeintegration.qrc
