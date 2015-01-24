include(../plugin.pri)

QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
}

SOURCES = \
    spellcheckplugin.cpp \
    spellchecker.cpp

HEADERS += \
    spellcheckplugin.h \
    spellchecker.h

RESOURCES += \
    main.qrc
