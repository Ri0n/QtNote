include(QtNoteMacro)

add_qtnote_plugin(tomboy "Allows to load notes from Tomboy" ON unix macosx windows)

add_library(tomboy ${LIB_TYPE}
    ${QTNOTE_COMMON_PLUGIN_SRC}
    tomboydata.cpp
    tomboydata.h
    tomboyplugin.cpp
    tomboyplugin.h
    tomboy.qrc
    tomboystorage.cpp
    tomboystorage.h
    )
find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS Xml REQUIRED)
target_link_libraries(tomboy ${EXTRA_LINK_TARGET} Qt${QT_DEFAULT_MAJOR_VERSION}::Core Qt${QT_DEFAULT_MAJOR_VERSION}::Gui Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets Qt${QT_DEFAULT_MAJOR_VERSION}::Xml)

install_qtnote_plugin(tomboy)
