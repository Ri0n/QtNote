cmake_minimum_required(VERSION 3.10.0)
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
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME qtnote)

target_link_libraries(${PROJECT_NAME} ${qtnote_lib} SingleApplication::SingleApplication)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
if(NOT WIN32)
    install(FILES ${CMAKE_SOURCE_DIR}/qtnote.desktop DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/applications)
    install(FILES ${CMAKE_SOURCE_DIR}/libqtnote/images/qtnote.svg DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/scalable/apps)
endif()
