include(QtNoteMacro)

find_package(KF${QT_VERSION_MAJOR}GlobalAccel CONFIG)
find_package(KF${QT_VERSION_MAJOR}Notifications CONFIG)
find_package(KF${QT_VERSION_MAJOR}WindowSystem CONFIG)
find_package(Plasma REQUIRED)

message(STATUS "Plasma version ${Plasma_VERSION_MAJOR}")

if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(KF${QT_VERSION_MAJOR}StatusNotifierItem CONFIG)
endif()

set(kdeintegration_buildable OFF)
if(KF${QT_VERSION_MAJOR}GlobalAccel_FOUND AND KF${QT_VERSION_MAJOR}Notifications_FOUND AND KF${QT_VERSION_MAJOR}WindowSystem_FOUND)
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

target_link_libraries(kdeintegration qtnote Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets
    KF${QT_VERSION_MAJOR}::GlobalAccel KF${QT_VERSION_MAJOR}::Notifications KF${QT_VERSION_MAJOR}::WindowSystem)

if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    target_link_libraries(kdeintegration KF${QT_VERSION_MAJOR}::StatusNotifierItem)
endif()

if("${KF${QT_VERSION_MAJOR}WindowSystem_VERSION}" VERSION_LESS_EQUAL "5.92") # Специфично для Ubuntu 22.04
    target_compile_definitions(kdeintegration PRIVATE OLD_K_FORCE_ACTIVATE)
endif()

if (UNIXLIKE)
    if (${QT_VERSION_MAJOR} LESS 6)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS X11Extras REQUIRED)
        target_link_libraries(kdeintegration Qt${QT_VERSION_MAJOR}::X11Extras)
    endif()
endif()

install(TARGETS kdeintegration LIBRARY DESTINATION ${PLUGINSDIR} COMPONENT Libraries NAMELINK_COMPONENT Development)


# Plasmoid stuff

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMSetupVersion)
include(ECMOptionalAddSubdirectory)
include(FindPkgConfig)
include(ECMDeprecationSettings)
include(ECMQmlModule)
include(ECMGenerateExportHeader)

find_package(KF6Kirigami ${KF6_MIN_VERSION} CONFIG REQUIRED)
set_package_properties(KF6Kirigami PROPERTIES
    DESCRIPTION "A QtQuick based components set"
    PURPOSE "Required for the runtime of the plasmoid"
    TYPE RUNTIME
)

find_package(KF6KirigamiAddons CONFIG REQUIRED)
set_package_properties(KirigamiAddons PROPERTIES
    DESCRIPTION "Extra controls for Kirigami applications"
    PURPOSE "Required for the runtime of the plasmoid"
    TYPE RUNTIME
)

find_package(KF6ItemModels CONFIG REQUIRED)
set_package_properties(KItemModels PROPERTIES
    DESCRIPTION "Set of item models extending the Qt model-view framework"
    PURPOSE "Required at runtime for handling complex data models"
    TYPE RUNTIME
)


plasma_install_package(plasmoid/package com.github.ri0n.qtnote)

add_library(plasmoid_qtnote MODULE
    plasmoid/plasmoid.cpp  # Основной исходный файл плагина
    plasmoid/plasmoid.h
   #plasmoid/qml.qrc  # QML ресурсы
)

# Связываем плазмоид с библиотеками и qtnote
target_link_libraries(plasmoid_qtnote qtnote Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Qml Plasma)

if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    # Поддержка для Plasma 6
    target_compile_definitions(plasmoid_qtnote PRIVATE PLASMA6_SUPPORT)
endif()

install(TARGETS plasmoid_qtnote DESTINATION ${PLUGINSDIR}/plasma/plasmoids)  # Установка плазмоида

# Уведомляем основной плагин о возможности управлять плазмоидом
target_compile_definitions(kdeintegration PRIVATE ENABLE_PLASMOID_CONTROL)
