cmake_minimum_required(VERSION 3.10.0)
include(QtNoteMacro)
project(qtnoteexe VERSION ${QTNOTE_VERSION} LANGUAGES CXX)

add_executable(qtnoteexe
main.cpp
)
set_target_properties(qtnoteexe PROPERTIES OUTPUT_NAME qtnote)

target_link_libraries(qtnoteexe qtnote SingleApplication::SingleApplication)

install(TARGETS qtnoteexe DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES ${CMAKE_SOURCE_DIR}/qtnote.desktop DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/applications)
install(FILES ${CMAKE_SOURCE_DIR}/libqtnote/images/qtnote.svg DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/scalable/apps)
