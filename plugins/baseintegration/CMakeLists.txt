include(QtNoteMacro)

add_qtnote_plugin(baseintegration "Base (fallback) desktop integration plugin" ON unix macosx windows)

set (QXT_SRC 3rdparty/qxt/qxtglobalshortcut.h
    3rdparty/qxt/qxtglobalshortcut.cpp
    3rdparty/qxt/qxtglobal.h
    3rdparty/qxt/qxtglobalshortcut_p.h)
if (APPLE)
    list(APPEND QXT_SRC 3rdparty/qxt/qxtglobalshortcut_mac.cpp)
endif()
if (UNIX AND NOT APPLE)
    list(APPEND QXT_SRC 3rdparty/qxt/qxtglobalshortcut_x11.cpp)
endif()
if (WIN32)
    list(APPEND QXT_SRC 3rdparty/qxt/qxtglobalshortcut_win.cpp)
endif()
add_library(baseintegration ${LIB_TYPE}
    ${QTNOTE_COMMON_PLUGIN_SRC}
    ${QXT_SRC}
    baseintegration.h
    baseintegration.cpp
    baseintegrationtray.h
    baseintegrationtray.cpp
    baseintegration.qrc
    )
target_include_directories(baseintegration PRIVATE 3rdparty/qxt)
target_link_libraries(baseintegration ${EXTRA_LINK_TARGET} Qt5::Core Qt5::Gui Qt5::Widgets)
if (UNIX)
    if (APPLE)
        target_link_libraries(baseintegration "-framework Carbon")
    else()
        find_package(Qt5 COMPONENTS X11Extras REQUIRED)
        target_link_libraries(baseintegration Qt5::X11Extras)
    endif()
endif()

install(TARGETS baseintegration DESTINATION ${PLUGINSDIR})
