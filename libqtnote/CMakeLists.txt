cmake_minimum_required(VERSION 3.10.0)
include(QtNoteMacro)
project(libqtnote VERSION ${QTNOTE_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB libqtnote_SRCS "./*.cpp")
file(GLOB libqtnote_HDRS "./*.h")
file(GLOB libqtnote_UIS "./*.ui")

if(WIN32)
    add_definitions(-DQTNOTE_EXPORT)
endif()

add_library(qtnote SHARED
    ${libqtnote_HDRS}
    ${libqtnote_SRCS}
    ${libqtnote_UIS}
    main.qrc
)

set(qtnote_lib qtnote PARENT_SCOPE)

target_include_directories(qtnote PRIVATE "${CMAKE_SOURCE_DIR}/plugins" "${CMAKE_BINARY_DIR}")
target_include_directories(qtnote PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/qtnote>  # <prefix>/include/qnote
    )
if(WIN32)
set_target_properties(qtnote
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}"
)
endif()
target_link_libraries(qtnote PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport)

install(TARGETS qtnote DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
