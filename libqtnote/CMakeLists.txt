cmake_minimum_required(VERSION 3.12.0)
include(QtNoteMacro)
project(libqtnote VERSION ${QTNOTE_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB libqtnote_SRCS "./*.cpp")
file(GLOB libqtnote_HDRS "./*.h")
file(GLOB libqtnote_UIS "./*.ui")

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
  PUBLIC_HEADER "${libqtnote_HDRS};${PROJECT_BINARY_DIR}/qtnote_export.h"
  VERSION ${QTNOTE_VERSION}
  SOVERSION ${QTNOTE_VERSION_MAJOR}
)
if(WIN32)
set_target_properties(qtnote
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}"
)
endif()
target_link_libraries(qtnote PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::PrintSupport)
if (APPLE)
    target_link_libraries(qtnote PUBLIC "-framework CoreFoundation" "-framework Carbon")
endif()

install(TARGETS qtnote
  LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT Libraries
    NAMELINK_COMPONENT Development
  PUBLIC_HEADER
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtnote"
    COMPONENT Development
)
