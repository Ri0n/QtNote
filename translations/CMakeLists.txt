project(translations)

find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS LinguistTools REQUIRED)

file(GLOB qtnote_TRS "${CMAKE_CURRENT_SOURCE_DIR}/qtnote_*.ts")
if(WIN32)
    set_source_files_properties(${qtnote_TRS} PROPERTIES OUTPUT_LOCATION "${EXECUTABLE_OUTPUT_PATH}/translations")
endif()

if(Qt${QT_DEFAULT_MAJOR_VERSION}LinguistTools_VERSION VERSION_LESS 6.7.0)
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
            COMMAND ${Qt${QT_DEFAULT_MAJOR_VERSION}_LUPDATE_EXECUTABLE}
            ARGS -no-obsolete -recursive ${PROJECT_DIRS} -ts ${_LANG}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            VERBATIM
        )
    endforeach()
    qt_add_translation(qtnote_QM ${qtnote_TRS})

    add_custom_target(build_translations ALL DEPENDS ${qtnote_QM})
else()
    set(LANGS_SOURCE_TARGETS)
    foreach(trg baseintegration gnome kdeintegration spellchecker tomboy macosx)
        if(TARGET ${trg})
            list(APPEND LANGS_SOURCE_TARGETS ${trg})
        endif()
    endforeach()
    qt_add_translations(TARGETS qtnoteexe
                        SOURCE_TARGETS qtnoteexe qtnote ${LANGS_SOURCE_TARGETS}
                        TS_FILE_BASE qtnote
                        TS_FILES ${qtnote_TRS}
                        QM_FILES_OUTPUT_VARIABLE qtnote_QM)
endif()

install(FILES ${qtnote_QM} DESTINATION ${TRANSLATIONSDIR} COMPONENT Executable)
