include(QtNoteMacro)

add_qtnote_plugin(ubuntu "Ubuntu integration")

add_library(ubuntu SHARED
    ${QTNOTE_COMMON_PLUGIN_SRC}
    ubuntu.cpp
    ubuntu.h
    ubuntu.pro
    ubuntu.qrc
    ubuntutray.cpp
    ubuntutray.h
    x11util.cpp
    x11util.h
    )
target_link_libraries(ubuntu Qt5::Core Qt5::Gui Qt5::Widgets)
if (UNIX AND NOT APPLE)
    find_package(Qt5 COMPONENTS X11Extras REQUIRED)
    target_link_libraries(ubuntu Qt5::X11Extras)
endif()

install(TARGETS ubuntu DESTINATION ${PLUGINSDIR})
