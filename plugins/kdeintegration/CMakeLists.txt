include(QtNoteMacro)

find_package(KF${QT_DEFAULT_MAJOR_VERSION}GlobalAccel CONFIG)
find_package(KF${QT_DEFAULT_MAJOR_VERSION}Notifications CONFIG)
find_package(KF${QT_DEFAULT_MAJOR_VERSION}WindowSystem CONFIG)
if (${QT_DEFAULT_MAJOR_VERSION} GREATER_EQUAL 6)
    find_package(KF${QT_DEFAULT_MAJOR_VERSION}StatusNotifierItem CONFIG)
    #find_package(KF${QT_DEFAULT_MAJOR_VERSION}X11Extras CONFIG)
endif()

set(kdeintegration_buildable OFF)
if(KF${QT_DEFAULT_MAJOR_VERSION}GlobalAccel_FOUND AND KF${QT_DEFAULT_MAJOR_VERSION}Notifications_FOUND AND KF${QT_DEFAULT_MAJOR_VERSION}WindowSystem_FOUND)
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
target_link_libraries(kdeintegration qtnote Qt${QT_DEFAULT_MAJOR_VERSION}::Core Qt${QT_DEFAULT_MAJOR_VERSION}::Gui Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets KF${QT_DEFAULT_MAJOR_VERSION}::GlobalAccel
    KF${QT_DEFAULT_MAJOR_VERSION}::Notifications KF${QT_DEFAULT_MAJOR_VERSION}::WindowSystem)
if (${QT_DEFAULT_MAJOR_VERSION} GREATER_EQUAL 6)
    target_link_libraries(kdeintegration KF${QT_DEFAULT_MAJOR_VERSION}::StatusNotifierItem)
endif()

if("${KF${QT_DEFAULT_MAJOR_VERSION}WindowSystem_VERSION}" VERSION_LESS_EQUAL "5.92") # 5.92 - un ubuntu 22.04. maybe a little wrong
    target_compile_definitions(kdeintegration PRIVATE OLD_K_FORCE_ACTIVATE)
endif()
if (UNIXLIKE)
    if (${QT_DEFAULT_MAJOR_VERSION} LESS 6)
        find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS X11Extras REQUIRED)
        target_link_libraries(kdeintegration Qt${QT_DEFAULT_MAJOR_VERSION}::X11Extras)
    # else()
    #     target_link_libraries(kdeintegration KF${QT_DEFAULT_MAJOR_VERSION}::X11Extras)
    endif()
endif()

install_qtnote_plugin(kdeintegration)
