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
find_package(Qt5 COMPONENTS Xml REQUIRED)
target_link_libraries(tomboy ${EXTRA_LINK_TARGET} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml)

install(TARGETS tomboy DESTINATION ${PLUGINSDIR})
