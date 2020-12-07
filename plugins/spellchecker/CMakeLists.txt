include(QtNoteMacro)

add_qtnote_plugin(spellchecker "Check for spelling errors")

find_package(PkgConfig REQUIRED)
pkg_check_modules(Hunspell REQUIRED IMPORTED_TARGET hunspell)

add_library(spellchecker SHARED
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
target_link_libraries(spellchecker Qt5::Core Qt5::Gui Qt5::Widgets PkgConfig::Hunspell)

install(TARGETS spellchecker DESTINATION ${PLUGINSDIR})
