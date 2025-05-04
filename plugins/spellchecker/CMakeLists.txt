include(QtNoteMacro)

if("${CMAKE_MODULE_PATH}" MATCHES "^.*conan.*$")
    message(STATUS "Conan detected. Trying to get hunspell from there")
    find_package(hunspell)
    set(HUNSPELL_DEP hunspell::hunspell)
    set(HUNSPEL_ENABLED ${hunspell_FOUND})
else()
    qtnote_optional_pkgconfig(Hunspell IMPORTED_TARGET hunspell)
    set(HUNSPELL_DEP  PkgConfig::Hunspell)
    set(HUNSPEL_ENABLED ${Hunspell_FOUND})
endif()
find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS Network)
if (NOT Qt${QT_DEFAULT_MAJOR_VERSION}Network_FOUND)
    set(HUNSPEL_ENABLED FALSE)
endif()


add_qtnote_plugin(spellchecker "Check for spelling errors" ${HUNSPEL_ENABLED} unix macosx windows SOURCES
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

target_sources(spellchecker
  PRIVATE
    hunspelldownloader.h hunspelldownloader.cpp
    dictionarydownloader.h dictionarydownloader.cpp
)


if(WIN32)
    add_compile_definitions(spellchecker PRIVATE DICTSUBDIR=/spellcheck/myspell)
endif()

target_link_libraries(spellchecker
    ${EXTRA_LINK_TARGET}
    Qt${QT_DEFAULT_MAJOR_VERSION}::Core
    Qt${QT_DEFAULT_MAJOR_VERSION}::Gui
    Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets
    Qt${QT_DEFAULT_MAJOR_VERSION}::Network
    ${HUNSPELL_DEP}
)

install_qtnote_plugin(spellchecker)
