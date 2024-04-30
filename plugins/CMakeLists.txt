project(plugins)

set(plugins_list
    baseintegration
    kdeintegration
    macosx
    spellchecker
    tomboy
    gnome
)

foreach(plugin ${plugins_list})
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${plugin}")
        add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/${plugin}")
    else()
        message(FATAL_ERROR "Invalid plugins list. Stumbled on ${plugin}")
    endif()
endforeach()
