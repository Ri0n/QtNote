include(../plugin.pri)

QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
}

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += hunspell
}

win32 {
    LIBS += -L$$HUNSPELL_DIR/lib -lhunspell
    INCLUDEPATH += $$HUNSPELL_DIR/include
}

SOURCES = \
    spellcheckplugin.cpp \
    hunspellengine.cpp \
    engineinterface.cpp \
    settingsdlg.cpp

HEADERS += \
    spellcheckplugin.h \
    hunspellengine.h \
    engineinterface.h \
    settingsdlg.h

RESOURCES += \
    spellchecker.qrc

FORMS += \
    settingsdlg.ui
