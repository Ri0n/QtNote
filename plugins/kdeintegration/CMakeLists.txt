include(QtNoteMacro)

set(QT_DEFAULT_MAJOR_VERSION ${QT_VERSION_MAJOR})

set(qtnote_plasmoid_buildable OFF)
if(UNIXLIKE AND QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS DBus Qml)
    find_package(ECM QUIET NO_MODULE)
    if(ECM_FOUND)
        set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
        find_package(KF6Config QUIET)
        find_package(KF6CoreAddons QUIET)
        find_package(Plasma 6.0 QUIET)
        if(KF6Config_FOUND AND KF6CoreAddons_FOUND AND Plasma_FOUND)
            set(qtnote_plasmoid_buildable ON)
        endif()
    endif()
endif()

if(qtnote_plasmoid_buildable)
    qt_policy(SET QTP0001 NEW)
    include(KDEInstallDirs)
    include("${Plasma_DIR}/PlasmaMacros.cmake")

    set(qtnote_plasmoid_id com.github.ri0n.qtnote)

    install(
        DIRECTORY plasmoid/package/
        DESTINATION ${KDE_INSTALL_DATADIR}/plasma/plasmoids/${qtnote_plasmoid_id}
        COMPONENT Executable
        PATTERN ".svn" EXCLUDE
        PATTERN "*.qmlc" EXCLUDE
        PATTERN "CMakeLists.txt" EXCLUDE
        PATTERN "Messages.sh" EXCLUDE
        PATTERN "dummydata" EXCLUDE
    )

    set(PLASMA_SKIP_APPLET_INSTALL ON)
    plasma_add_applet(${qtnote_plasmoid_id}
        QML_SOURCES
            plasmoid/package/contents/ui/main.qml
            plasmoid/package/contents/ui/CompactRepresentation.qml
            plasmoid/package/contents/ui/FullRepresentation.qml
        CPP_SOURCES
            plasmoid/appletplugin.cpp
            plasmoid/plasmoid.cpp
            plasmoid/plasmoid.h
        QML_ARGS
            VERSION 1.0
    )
    unset(PLASMA_SKIP_APPLET_INSTALL)

    add_custom_target(plasmoid_qtnote DEPENDS ${qtnote_plasmoid_id})
    set_target_properties(${qtnote_plasmoid_id} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plasma/applets"
    )
    target_include_directories(${qtnote_plasmoid_id} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/plasmoid)
    target_link_libraries(${qtnote_plasmoid_id} PRIVATE
        Qt6::Core Qt6::DBus Qt6::Qml
        KF6::CoreAddons Plasma::Plasma
    )
    install(
        TARGETS ${qtnote_plasmoid_id}
        DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets
        COMPONENT Executable
    )

    if(QTNOTE_DEVEL)
        set(qtnote_devel_qml_module_dir
            ${CMAKE_CURRENT_BINARY_DIR}/qml/QtNote
        )
        qt_add_qml_module(plasmoid_qtnote_devel
            URI QtNote
            VERSION 1.0
            PLUGIN_TARGET plasmoid_qtnote_devel
            OUTPUT_DIRECTORY ${qtnote_devel_qml_module_dir}
            SOURCES
                plasmoid/plasmoid.cpp
                plasmoid/plasmoid.h
        )
        target_include_directories(plasmoid_qtnote_devel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/plasmoid)
        target_link_libraries(plasmoid_qtnote_devel PRIVATE Qt6::Core Qt6::DBus Qt6::Qml)
        target_compile_definitions(plasmoid_qtnote_devel PRIVATE
            QTNOTE_DEVEL_EXECUTABLE="$<TARGET_FILE:qtnoteexe>"
            QTNOTE_DEVEL_TRANSLATIONSDIR="${CMAKE_BINARY_DIR}/translations"
        )
    endif()
else()
    message(STATUS "Plasma 6 development files not found; the QtNote plasmoid will not be built")
endif()

find_package(KF${QT_VERSION_MAJOR}GlobalAccel CONFIG)
find_package(KF${QT_VERSION_MAJOR}Notifications CONFIG)
find_package(KF${QT_VERSION_MAJOR}WindowSystem CONFIG)

set(kdeintegration_buildable OFF)
if(KF${QT_VERSION_MAJOR}GlobalAccel_FOUND AND KF${QT_VERSION_MAJOR}Notifications_FOUND AND KF${QT_VERSION_MAJOR}WindowSystem_FOUND)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus REQUIRED)
    set(kdeintegration_buildable ON)
endif()
set(kdeintegration_auto_disabled_cache_key QTNOTE_PLUGIN_AUTO_DISABLED_kdeintegration)
if(NOT kdeintegration_buildable)
    if(QTNOTE_PLUGIN_ENABLE_kdeintegration)
        set(${kdeintegration_auto_disabled_cache_key} ON CACHE INTERNAL "kdeintegration was disabled automatically")
    endif()
    set(QTNOTE_PLUGIN_ENABLE_kdeintegration OFF CACHE BOOL "Enable QtNote plugin: Integration with KDE Plasma" FORCE)
elseif(${kdeintegration_auto_disabled_cache_key} OR (DEFINED CACHE{QTNOTE_PLUGIN_ENABLE_kdeintegration} AND NOT QTNOTE_PLUGIN_ENABLE_kdeintegration))
    set(QTNOTE_PLUGIN_ENABLE_kdeintegration ON CACHE BOOL "Enable QtNote plugin: Integration with KDE Plasma" FORCE)
    unset(${kdeintegration_auto_disabled_cache_key} CACHE)
endif()

add_qtnote_plugin(kdeintegration "Integration with KDE Plasma" ${kdeintegration_buildable} unix SOURCES
    kdeintegration.cpp
    kdeintegration.h
    kdeintegration.qrc
    kdeintegrationtray.cpp
    kdeintegrationtray.h
)

target_link_libraries(kdeintegration qtnote Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::DBus
    KF${QT_VERSION_MAJOR}::GlobalAccel KF${QT_VERSION_MAJOR}::Notifications KF${QT_VERSION_MAJOR}::WindowSystem)

if("${KF${QT_VERSION_MAJOR}WindowSystem_VERSION}" VERSION_LESS_EQUAL "5.92") # Специфично для Ubuntu 22.04
    target_compile_definitions(kdeintegration PRIVATE OLD_K_FORCE_ACTIVATE)
endif()

if (UNIXLIKE)
    if (${QT_VERSION_MAJOR} LESS 6)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS X11Extras REQUIRED)
        target_link_libraries(kdeintegration Qt${QT_VERSION_MAJOR}::X11Extras)
    endif()
endif()

if(QTNOTE_DEVEL AND qtnote_plasmoid_buildable)
    target_compile_definitions(kdeintegration PRIVATE
        QTNOTE_DEVEL_PLASMOID_PACKAGE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/plasmoid/package"
        QTNOTE_DEVEL_PLASMOID_QML_DIR="${qtnote_devel_qml_module_dir}"
        QTNOTE_DEVEL_PLASMOID_QML_INSTALL_DIR="${KDE_INSTALL_QMLDIR}/com/github/ri0n/qtnote"
    )
endif()

install_qtnote_plugin(kdeintegration)
