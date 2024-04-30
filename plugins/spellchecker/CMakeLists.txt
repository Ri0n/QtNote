include(QtNoteMacro)

qtnote_optional_pkgconfig(Hunspell IMPORTED_TARGET hunspell)

add_qtnote_plugin(spellchecker "Check for spelling errors" ${Hunspell_FOUND} unix macosx windows)

if(WIN32)
    add_definitions(-DDICTSUBDIR="/spellcheck/myspell")
endif()

add_library(spellchecker ${LIB_TYPE}
    ${QTNOTE_COMMON_PLUGIN_SRC}
    engineinterface.cpp
    engineinterface.h
    hunspellengine.cpp
    hunspellengine.h
    settingsdlg.cpp
    settingsdlg.h
    settingsdlg.ui
    spellchecker.qrc
    spellcheckplugin.cpp
    spellcheckplugin.h
    )
target_link_libraries(spellchecker ${EXTRA_LINK_TARGET} Qt${QT_DEFAULT_MAJOR_VERSION}::Core Qt${QT_DEFAULT_MAJOR_VERSION}::Gui Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets PkgConfig::Hunspell)

install(TARGETS spellchecker LIBRARY DESTINATION ${PLUGINSDIR} COMPONENT Libraries NAMELINK_COMPONENT Development)
