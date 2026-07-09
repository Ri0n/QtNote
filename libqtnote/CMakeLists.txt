cmake_minimum_required(VERSION 3.14.0)
include(QtNoteMacro)
project(libqtnote VERSION ${QTNOTE_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB libqtnote_SRCS "./*.cpp")
file(GLOB libqtnote_HDRS "./*.h")
file(GLOB libqtnote_UIS "./*.ui")
list(APPEND libqtnote_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/iconutils.cpp")
list(APPEND libqtnote_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/iconutils.h")
list(REMOVE_DUPLICATES libqtnote_SRCS)
list(REMOVE_DUPLICATES libqtnote_HDRS)
list(REMOVE_ITEM libqtnote_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/freedesktopnotifier.cpp")
list(REMOVE_ITEM libqtnote_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/freedesktopnotifier.h")
list(REMOVE_ITEM libqtnote_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/qtnotedbus.cpp")
list(REMOVE_ITEM libqtnote_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/qtnotedbus.h")

if(UNIXLIKE)
    list(APPEND libqtnote_SRCS freedesktopnotifier.cpp)
    list(APPEND libqtnote_HDRS freedesktopnotifier.h)
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus QUIET)
if(TARGET Qt${QT_VERSION_MAJOR}::DBus)
    list(APPEND libqtnote_SRCS qtnotedbus.cpp)
    list(APPEND libqtnote_HDRS qtnotedbus.h)
endif()
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Multimedia QUIET)

include(GenerateExportHeader)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
add_library(qtnote SHARED
    ${libqtnote_HDRS}
    ${libqtnote_SRCS}
    ${libqtnote_UIS}
    main.qrc
)
generate_export_header(qtnote)

set(qtnote_lib qtnote PARENT_SCOPE)

target_include_directories(qtnote PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../plugins" "${CMAKE_BINARY_DIR}")
target_include_directories(qtnote PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include/qtnote>  # <prefix>/include/qnote
    )

set_target_properties(qtnote PROPERTIES
  VERSION ${QTNOTE_VERSION}
  SOVERSION ${QTNOTE_VERSION_MAJOR}
)
if(WIN32)
    set_target_properties(qtnote PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}"
    )
else()
    set_target_properties(qtnote PROPERTIES
        PUBLIC_HEADER "${libqtnote_HDRS};${PROJECT_BINARY_DIR}/qtnote_export.h"
    )
endif()
target_link_libraries(qtnote PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::PrintSupport)
if(TARGET Qt${QT_VERSION_MAJOR}::DBus)
    target_link_libraries(qtnote PUBLIC Qt${QT_VERSION_MAJOR}::DBus)
    target_compile_definitions(qtnote PUBLIC QTNOTE_DBUS_AVAILABLE)
endif()
if(TARGET Qt${QT_VERSION_MAJOR}::Multimedia)
    target_link_libraries(qtnote PUBLIC Qt${QT_VERSION_MAJOR}::Multimedia)
    target_compile_definitions(qtnote PUBLIC QTNOTE_MULTIMEDIA_AVAILABLE)
endif()
if (APPLE)
    target_link_libraries(qtnote PUBLIC "-framework CoreFoundation" "-framework Carbon")
endif()

if(WIN32)
    # Install only the DLL for Windows
    install(TARGETS qtnote
        RUNTIME
            DESTINATION "."
            COMPONENT Libraries
    )
    windeployqt(qtnote)
else()
    install(TARGETS qtnote
      LIBRARY
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT Libraries
        NAMELINK_COMPONENT Development
      PUBLIC_HEADER
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtnote"
        COMPONENT Development
    )
endif()
