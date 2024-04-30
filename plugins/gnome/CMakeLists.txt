include(QtNoteMacro)

add_qtnote_plugin(gnome "Gnome integration" ON unix)

add_library(gnome ${LIB_TYPE}
    ${QTNOTE_COMMON_PLUGIN_SRC}
    gnome.cpp
    gnome.h
    gnome.qrc
    gnometray.cpp
    gnometray.h
    x11util.cpp
    x11util.h
    )
target_link_libraries(gnome ${EXTRA_LINK_TARGET} Qt${QT_DEFAULT_MAJOR_VERSION}::Core Qt${QT_DEFAULT_MAJOR_VERSION}::Gui Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets)
if (UNIXLIKE  AND ${QT_DEFAULT_MAJOR_VERSION} LESS 6)
    find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS X11Extras REQUIRED)
    target_link_libraries(gnome Qt${QT_DEFAULT_MAJOR_VERSION}::X11Extras)
endif()

install(TARGETS gnome LIBRARY DESTINATION ${PLUGINSDIR} COMPONENT Libraries NAMELINK_COMPONENT Development)
