project(plugins)

set(plugins_list
    baseintegration
    kdeintegration
    spellchecker
    tomboy
    ubuntu
)

foreach(plugin ${plugins_list})
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/${plugin}")
endforeach()
