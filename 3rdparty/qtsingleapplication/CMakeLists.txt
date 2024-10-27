cmake_minimum_required(VERSION 3.5)

project(QtSingleApplication)

option(QT_DEFAULT_MAJOR_VERSION "Set default Qt major version (5 or 6)" "")
option(QT_SINGLE_APPLICATION_INSTALL "Disable installation of QtSingleApplication" ON)
option(QT_SINGLE_APPLICATION_WIDGET "Enable QApplication (Widget-based Application)" ON)

set(QT_COMPONENTS Core Gui Network)
if(QT_SINGLE_APPLICATION_WIDGET)
    list(APPEND QT_COMPONENTS Widgets)
endif()

if (QT_DEFAULT_MAJOR_VERSION STREQUAL "6" OR NOT QT_DEFAULT_MAJOR_VERSION)
    find_package(Qt6 COMPONENTS ${QT_COMPONENTS} REQUIRED)
    set(QT_VERSION_MAJOR ${Qt6Core_VERSION_MAJOR})
endif()

if (QT_DEFAULT_MAJOR_VERSION STREQUAL "5" OR NOT QT_VERSION_MAJOR)
    find_package(Qt5 COMPONENTS ${QT_COMPONENTS} REQUIRED)
    set(QT_VERSION_MAJOR ${Qt5Core_VERSION_MAJOR})
endif()

if (NOT QT_VERSION_MAJOR)
    message(FATAL_ERROR "Could not find Qt5 or Qt6. Please install Qt or specify the correct QT_DEFAULT_MAJOR_VERSION.")
endif()

message(STATUS "Using Qt version ${QT_VERSION_MAJOR}")

set(SOURCES
    qtsinglecoreapplication.cpp
    qtlocalpeer.cpp
)

set(HEADERS
    qtsinglecoreapplication.h
    qtlocalpeer.h
)

if(QT_SINGLE_APPLICATION_WIDGET)
    list(APPEND SOURCES qtsingleapplication.cpp)
    list(APPEND HEADERS qtsingleapplication.h)
endif()

add_library(QtSingleApplication ${SOURCES} ${HEADERS})
target_include_directories(QtSingleApplication PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(QT_SINGLE_APPLICATION_INSTALL)
    install(TARGETS QtSingleApplication DESTINATION lib)
    install(FILES ${HEADERS} DESTINATION include/QtSingleApplication)
endif()

target_link_libraries(QtSingleApplication PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Network)
if(QT_SINGLE_APPLICATION_WIDGET)
    target_link_libraries(QtSingleApplication PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
endif()
