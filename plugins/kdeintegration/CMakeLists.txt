include(QtNoteMacro)

find_package(KF5GlobalAccel CONFIG)
find_package(KF5Notifications CONFIG)
find_package(KF5WindowSystem CONFIG)

set(kdeintegration_buildable OFF)
if(KF5GlobalAccel_FOUND AND KF5Notifications_FOUND AND KF5WindowSystem_FOUND)
    set(kdeintegration_buildable ON)
endif()

add_qtnote_plugin(kdeintegration "Integration with KDE Plasma" ${kdeintegration_buildable} unix)

add_library(kdeintegration ${LIB_TYPE}
    ${QTNOTE_COMMON_PLUGIN_SRC}
    kdeintegration.cpp
    kdeintegration.h
    kdeintegration.qrc
    kdeintegrationtray.cpp
    kdeintegrationtray.h
    )
target_link_libraries(kdeintegration Qt5::Core Qt5::Gui Qt5::Widgets KF5::GlobalAccel
    KF5::Notifications KF5::WindowSystem)
if (UNIXLIKE)
    find_package(Qt5 COMPONENTS X11Extras REQUIRED)
    target_link_libraries(kdeintegration Qt5::X11Extras)
endif()

install(TARGETS kdeintegration DESTINATION ${PLUGINSDIR})
