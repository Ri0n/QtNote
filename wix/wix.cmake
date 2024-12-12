find_program(WINDEPLOYQT_EXECUTABLE windeployqt)
if(NOT WINDEPLOYQT_EXECUTABLE)
    message(WARNING "windeployqt not found! Dependencies for standalone deployment will not be gathered.")
endif()

find_program(WIX_EXECUTABLE wix.exe)
if(NOT WIX_EXECUTABLE)
    message(WARNING "WiX not found! Installer won't be built.")
endif()
if(NOT WIX_EXECUTABLE OR NOT WINDEPLOYQT_EXECUTABLE)
    return()
endif()

set(WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/wix/installer.wxs.in")
set(WIX_BUNDLE_TEMPLATE "${CMAKE_SOURCE_DIR}/wix/QtNoteInstaller.wixbundle.in")
set(WIX_OUTPUT "${CMAKE_BINARY_DIR}/installer.wxs")
set(WIX_BUNDLE_OUTPUT "${CMAKE_BINARY_DIR}/QtNoteInstaller.wixbundle")
set(QTNOTE_MSI ${CMAKE_BINARY_DIR}/QtNote.msi)

set(QTNOTE_INSTALLER_EXE QtNote.Installer-${QTNOTE_VERSION}.exe)
set(QTNOTE_INSTALLER_EXE_PATH ${CMAKE_BINARY_DIR}/${QTNOTE_INSTALLER_EXE})

set(LICENSE_TXT "${CMAKE_SOURCE_DIR}/license.txt")
set(LICENSE_RTF "${CMAKE_BINARY_DIR}/GPLv3.rtf")
set(LOGO_FILE "${CMAKE_SOURCE_DIR}/src/win/pen64.ico")

add_custom_command(
    OUTPUT ${LICENSE_RTF}
    COMMAND ${CMAKE_COMMAND} -E copy ${LICENSE_TXT} ${LICENSE_RTF}
    COMMAND ${CMAKE_COMMAND} -E echo "{\\rtf1\\ansi\\deff0" > ${LICENSE_RTF}
    COMMAND ${CMAKE_COMMAND} -E echo "\\par" >> ${LICENSE_RTF}
    COMMAND ${CMAKE_COMMAND} -E echo "{\\fonttbl {\\f0 Courier;}}" >> ${LICENSE_RTF}
    COMMAND ${CMAKE_COMMAND} -E cat ${LICENSE_TXT} >> ${LICENSE_RTF}
    COMMAND ${CMAKE_COMMAND} -E echo "}" >> ${LICENSE_RTF}
    COMMENT "Converting GPLv3.txt to GPLv3.rtf"
)

add_custom_target(generate_rtf ALL DEPENDS ${LICENSE_RTF})

function(path_to_id_name path)
    file(RELATIVE_PATH rel_file "${CMAKE_INSTALL_PREFIX}" "${path}")
    #string(REPLACE "/" "\\" rel_file "${rel_file}")
    get_filename_component(file_name "${rel_file}" NAME)
    #get_filename_component(file_dir "${rel_file}" PATH)
    #string(REPLACE "/" "\\" file_dir "${file_dir}")
    string(TOUPPER "${file_name}" file_id)
    string(REPLACE "." "_" file_id "${file_id}")
    #string(REPLACE "\\" "_" component_id "${component_id}")
    return(PROPAGATE file_id file_name)
endfunction()

function(generate_wxs_dir dir_to_glob indent)
    file(GLOB all_files "${dir_to_glob}/*")
    set(files "")
    set(dirs "")
    foreach(file ${all_files})
        if(IS_DIRECTORY ${file})
            list(APPEND dirs ${file})
        else()
            list(APPEND files ${file})
        endif()
    endforeach()
    set(components "")
    set(component_refs "")
    foreach(file ${files})
        path_to_id_name(${file})
        if(${file_id} STREQUAL VC_REDIST_X64_EXE)
            continue()
        endif()
        list(APPEND components "
            ${indent}<Component Id=\"${file_id}\" Guid=\"*\" Bitness=\"always64\">
            ${indent}    <File Id=\"${file_id}\" Name=\"${file_name}\" Source=\"${file}\" />
            ${indent}</Component>")
        list(APPEND component_refs "
          <ComponentRef Id=\"${file_id}\" />")
    endforeach()
    foreach(dir ${dirs})
        generate_wxs_dir(${dir} "${indent}    ")
        path_to_id_name(${dir})
        list(APPEND components "
            ${indent}<Directory Id=\"${file_id}\" Name=\"${file_name}\">
            ${indent}    ${components_res}
            ${indent}</Directory>")
        list(APPEND component_refs ${component_refs_res})
    endforeach()

    set(components_res ${components})
    set(component_refs_res ${component_refs})
    return(PROPAGATE components_res component_refs_res)
endfunction()

function(generate_wxs output_file output_bundle_file)
    generate_wxs_dir(${CMAKE_INSTALL_PREFIX} "")
    set(COMPONENTS ${components_res})
    set(COMPONENT_REFS ${component_refs_res})
    set(UPGRADE_CODE "E12D6C7A-8F57-479B-8AE9-49C411B82843")
    configure_file(${WIX_TEMPLATE} ${output_file} @ONLY)
    configure_file(${WIX_BUNDLE_TEMPLATE} ${output_bundle_file} @ONLY)
    configure_file(${CMAKE_SOURCE_DIR}/src/win/pen64.ico ${CMAKE_BINARY_DIR}/pen64.ico COPYONLY)
endfunction()

generate_wxs(${WIX_OUTPUT} ${WIX_BUNDLE_OUTPUT})

add_custom_target(prepare_install
    COMMAND ${CMAKE_COMMAND} --build . --target install
    USES_TERMINAL
    COMMENT "Installing project into ${CMAKE_INSTALL_PREFIX}"
)

add_custom_target(package
    DEPENDS prepare_install
    COMMAND ${WIX_EXECUTABLE} build ${WIX_OUTPUT} -platform x64 -o ${QTNOTE_MSI}
    USES_TERMINAL
    COMMENT "Building MSI package with WiX 5"
)

# don't forget `wix extension add -g WixToolset.BootstrapperApplications.wixext`
add_custom_command(
    OUTPUT ${QTNOTE_INSTALLER_EXE}
    COMMAND ${WIX_EXECUTABLE} build ${WIX_BUNDLE_OUTPUT} -ext WixToolset.BootstrapperApplications.wixext -o ${QTNOTE_INSTALLER_EXE_PATH}
    DEPENDS ${WIX_BUNDLE_OUTPUT} ${QTNOTE_MSI} ${LICENSE_RTF}
    COMMENT "Building Burn Bootstrapper"
)

add_custom_target(burn_installer DEPENDS ${QTNOTE_INSTALLER_EXE_PATH} package)
