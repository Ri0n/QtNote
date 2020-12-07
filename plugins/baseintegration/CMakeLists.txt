include(QtNoteMacro)

add_qtnote_plugin(baseintegration "Base (fallback) descktop integration plugin")

qt5_add_resources(RC_SRC "baseintegration.qrc")
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
add_library(baseintegration SHARED
    ${RC_SRC}
    ${QXT_SRC}
    ${QTNOTE_COMMON_PLUGIN_SRC}
    baseintegration.h
    baseintegration.cpp
    baseintegrationtray.h
    baseintegrationtray.cpp
    )
target_include_directories(baseintegration PRIVATE ${CMAKE_BINARY_DIR} ${plugins_SOURCE_DIR}
    ${libqtnote_SOURCE_DIR} 3rdparty/qxt)
find_package(Qt5 COMPONENTS X11Extras REQUIRED)
target_link_libraries(baseintegration Qt5::Core Qt5::Gui Qt5::Widgets Qt5::X11Extras)
