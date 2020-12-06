include(QtNoteMacro)

add_qtnote_plugin(baseintegration "Base (fallback) descktop integration plugin")

qt5_add_resources(RC_SRC "baseintegration.qrc")
add_library(baseintegration MODULE
    ${RC_SRC}
    baseintegration.h
    baseintegration.cpp
    baseintegrationtray.h
    baseintegrationtray.cpp
    )
target_include_directories(baseintegration PRIVATE ${CMAKE_BINARY_DIR} ${plugins_SOURCE_DIR} ${libqtnote_SOURCE_DIR} 3rdparty/qxt)
target_link_libraries(baseintegration Qt5::Core Qt5::Gui Qt5::Widgets)
