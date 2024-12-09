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
target_link_libraries(baseintegration ${EXTRA_LINK_TARGET} Qt${QT_DEFAULT_MAJOR_VERSION}::Core Qt${QT_DEFAULT_MAJOR_VERSION}::Gui Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets)
if (UNIX)
    if (APPLE)
        target_link_libraries(baseintegration "-framework Carbon")
    elseif(${QT_DEFAULT_MAJOR_VERSION} LESS 6)
        find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS X11Extras REQUIRED)
        target_link_libraries(baseintegration Qt${QT_DEFAULT_MAJOR_VERSION}::X11Extras)
    endif()
endif()

install_qtnote_plugin(baseintegration)
