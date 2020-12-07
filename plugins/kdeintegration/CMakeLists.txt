include(QtNoteMacro)

find_package(KF5GlobalAccel CONFIG)
find_package(KF5Notifications CONFIG)
find_package(KF5WindowSystem CONFIG)
add_qtnote_plugin(kdeintegration "Integration with KDE Plasma")

add_library(kdeintegration SHARED
    ${QTNOTE_COMMON_PLUGIN_SRC}
    kdeintegration.cpp
    kdeintegration.h
    kdeintegration.qrc
    kdeintegrationtray.cpp
    kdeintegrationtray.h
    )
target_link_libraries(kdeintegration Qt5::Core Qt5::Gui Qt5::Widgets KF5::GlobalAccel
    KF5::Notifications KF5::WindowSystem)
if (UNIX AND NOT APPLE)
    find_package(Qt5 COMPONENTS X11Extras REQUIRED)
    target_link_libraries(kdeintegration Qt5::X11Extras)
endif()
