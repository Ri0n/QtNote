include(QtNoteMacro)

add_qtnote_plugin(gnome "Gnome integration" ON unix SOURCES
    gnome.cpp
    gnome.h
    gnome.qrc
    gnometray.cpp
    gnometray.h
    x11util.cpp
    x11util.h
)

target_link_libraries(gnome ${EXTRA_LINK_TARGET} Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets)
if (UNIXLIKE  AND ${QT_VERSION_MAJOR} LESS 6)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS X11Extras REQUIRED)
    target_link_libraries(gnome Qt${QT_VERSION_MAJOR}::X11Extras)
endif()
install_qtnote_plugin(gnome)
