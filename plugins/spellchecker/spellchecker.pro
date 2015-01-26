include(../plugin.pri)

QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
}

CONFIG += link_pkgconfig

PKGCONFIG += hunspell

SOURCES = \
    spellcheckplugin.cpp \
    spellchecker.cpp \
    hunspellengine.cpp \
    engineinterface.cpp

HEADERS += \
    spellcheckplugin.h \
    spellchecker.h \
    hunspellengine.h \
    engineinterface.h

RESOURCES += \
    spellchecker.qrc
