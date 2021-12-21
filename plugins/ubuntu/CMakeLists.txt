include(QtNoteMacro)

add_qtnote_plugin(ubuntu "Ubuntu integration" ON unix)

add_library(ubuntu ${LIB_TYPE}
    ${QTNOTE_COMMON_PLUGIN_SRC}
    ubuntu.cpp
    ubuntu.h
    ubuntu.qrc
    ubuntutray.cpp
    ubuntutray.h
    x11util.cpp
    x11util.h
    )
target_link_libraries(ubuntu Qt5::Core Qt5::Gui Qt5::Widgets)
if (UNIXLIKE)
    find_package(Qt5 COMPONENTS X11Extras REQUIRED)
    target_link_libraries(ubuntu Qt5::X11Extras)
endif()

install(TARGETS ubuntu DESTINATION ${PLUGINSDIR})
