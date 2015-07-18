include(../plugin.pri)

QT *= gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= widgets
}

DICTSUBDIR = /spellcheck/myspell
DEFINES += DICTSUBDIR=$$DICTSUBDIR

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += hunspell
}

win32 {
    !isEmpty(HUNSPELL_LIB) {
        LIBS += $$HUNSPELL_LIB
    } else {
        !isEmpty(HUNSPELL_DIR):LIBS += -L$$HUNSPELL_DIR/lib -lhunspell-1.3
    }
    !isEmpty(HUNSPELL_INC) {
        INCLUDEPATH += $$HUNSPELL_INC
    } else {
        !isEmpty(HUNSPELL_DIR):INCLUDEPATH += $$HUNSPELL_DIR/include
    }
    inst_dictdir.target = "spelldir"
    target.depends = inst_dictdir
    fulldictpath = \"$$shell_path($${WININST_PREFIX}$${DICTSUBDIR})\"
    inst_dictdir.commands = $${QMAKE_CHK_DIR_EXISTS} $${fulldictpath} $${QMAKE_MKDIR} $${fulldictpath}
    QMAKE_EXTRA_TARGETS += inst_dictdir
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
