project(langs)
file(GLOB qtnote_TRS "${CMAKE_CURRENT_SOURCE_DIR}/qtnote_*.ts")

find_package(Qt5 COMPONENTS LinguistTools REQUIRED)

set(PROJECT_DIRS
${CMAKE_SOURCE_DIR}
${qtnotexe_SOURCE_DIR}
${libqtnote_SOURCE_DIR}
${plugins_SOURCE_DIR}
)

#update translations with: make lupdate
add_custom_target(lupdate)
foreach(_LANG ${qtnote_TRS})
    get_filename_component(_SHORT_LANG ${_LANG} NAME_WE)
    string(REPLACE "@" "_" _SHORT_LANG ${_SHORT_LANG})
    add_custom_command(TARGET lupdate
        PRE_BUILD
        COMMAND ${Qt5_LUPDATE_EXECUTABLE}
        ARGS -no-obsolete -recursive ${PROJECT_DIRS} -ts ${_LANG}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        VERBATIM
    )
endforeach()

qt5_add_translation(qtnote_QM ${qtnote_TRS})
add_custom_target(build_translations ALL DEPENDS ${qtnote_QM})

install(FILES ${qtnote_QM} DESTINATION ${TRANSLATIONSDIR})
