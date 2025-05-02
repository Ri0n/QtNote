cmake_minimum_required(VERSION 3.14.0)
include(QtNoteMacro)
project(qtnoteexe VERSION ${QTNOTE_VERSION} LANGUAGES CXX)

if(WIN32)
    set(WINRESFILE "win/qtnote.rc")
endif()

set(MACOSX_BUNDLE_ICON_FILE qtnote)
set(APP_ICON_MACOSX ${CMAKE_CURRENT_SOURCE_DIR}/mac/qtnote.icns)
set_source_files_properties(${APP_ICON_MACOSX} PROPERTIES
       MACOSX_PACKAGE_LOCATION "Resources")

add_executable(${PROJECT_NAME}
    WIN32
    MACOSX_BUNDLE
    main.cpp
    ${WINRESFILE}
    ${APP_ICON_MACOSX}
)

#add_dependencies(${PROJECT_NAME} build_translations)
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME qtnote
    MACOSX_BUNDLE_BUNDLE_NAME QtNote
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/mac/Info.plist.in"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${QTNOTE_VERSION}" # version
    MACOSX_BUNDLE_BUNDLE_VERSION "${QTNOTE_VERSION}" # git tag
    MACOSX_BUNDLE_COPYRIGHT "Copyright (C) 2010-2024 Sergei Ilinykh"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.ri0n.qtnote"
)
if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}"
    )
endif()

target_link_libraries(${PROJECT_NAME} ${qtnote_lib} QtSingleApplication)

if(WIN32)
    install(TARGETS ${PROJECT_NAME} RUNTIME
            DESTINATION "."
            COMPONENT Executable)
    windeployqt(${PROJECT_NAME})
else(WIN32)
    install(TARGETS ${PROJECT_NAME} RUNTIME
            DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT Executable)
    install(FILES ${CMAKE_SOURCE_DIR}/qtnote.desktop DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/applications COMPONENT Executable)
    install(FILES ${CMAKE_SOURCE_DIR}/libqtnote/images/qtnote.svg DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/scalable/apps COMPONENT Executable)
endif()
