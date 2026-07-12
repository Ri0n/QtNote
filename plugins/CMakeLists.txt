project(plugins)

set(plugins_list
    baseintegration
    cinnamon
    gemini
    gnome
    kdeintegration
    macosx
    nextcloud
    openaiwhisper
    spellchecker
    tomboy
    xmpppubsub
)

foreach(plugin ${plugins_list})
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${plugin}")
        add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/${plugin}")
    else()
        message(FATAL_ERROR "Invalid plugins list. Stumbled on ${plugin}")
    endif()
endforeach()
