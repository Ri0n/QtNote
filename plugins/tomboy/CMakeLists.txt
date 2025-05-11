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
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Xml REQUIRED)
target_link_libraries(tomboy ${EXTRA_LINK_TARGET} Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Xml)

install(TARGETS tomboy LIBRARY DESTINATION ${PLUGINSDIR} COMPONENT Libraries NAMELINK_COMPONENT Development)
