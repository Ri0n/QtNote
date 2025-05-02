cmake_minimum_required(VERSION 3.14.0)
include(QtNoteMacro)
project(installer VERSION ${QTNOTE_VERSION})

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
set(LICENSE_RTF "GPLv3.rtf")
set(LOGO_FILE "${CMAKE_SOURCE_DIR}/src/win/pen64.ico")

configure_file(gen-wix.cmake.in gen-wix.cmake @ONLY)


add_custom_command(
    OUTPUT ${WIX_BUNDLE_OUTPUT} ${WIX_OUTPUT} ${LICENSE_RTF}
    COMMAND ${CMAKE_COMMAND}
        -DVERSION_FOR_WIX=${VERSION_FOR_WIX}
        -DUPGRADE_CODE=${UPGRADE_CODE}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -DWIX_TEMPLATE=${WIX_TEMPLATE}
        -DWIX_BUNDLE_TEMPLATE=${WIX_BUNDLE_TEMPLATE}
        -DWIX_OUTPUT=${WIX_OUTPUT}
        -DWIX_BUNDLE_OUTPUT=${WIX_BUNDLE_OUTPUT}
        -DLICENSE_TEMPLATE=${CMAKE_CURRENT_SOURCE_DIR}/GPLv3.rtf.in
        -DLICENSE_TXT=${LICENSE_TXT}
        -DLICENSE_RTF=${LICENSE_RTF}
        -DLOGO_FILE=${LOGO_FILE}
        -DQTNOTE_MSI=${QTNOTE_MSI}
        -P ${CMAKE_CURRENT_BINARY_DIR}/gen-wix.cmake
    COMMENT "Prepare installer files"
    DEPENDS gen-wix.cmake
)
add_custom_target(prepare_install
    DEPENDS ${WIX_OUTPUT} ${WIX_BUNDLE_OUTPUT} ${LICENSE_RTF}
)
set_target_properties(prepare_install PROPERTIES EXCLUDE_FROM_ALL TRUE)

add_custom_command(
    OUTPUT ${QTNOTE_MSI}
    COMMAND ${WIX_EXECUTABLE} build ${WIX_OUTPUT} -platform x64 -o ${QTNOTE_MSI}
    COMMENT "Building MSI package with WiX"
    DEPENDS ${WIX_OUTPUT}
)
add_custom_target(package
    DEPENDS ${QTNOTE_MSI}
)
set_target_properties(package PROPERTIES EXCLUDE_FROM_ALL TRUE)

# don't forget `wix extension add -g WixToolset.BootstrapperApplications.wixext`
add_custom_command(
    OUTPUT ${QTNOTE_INSTALLER_EXE_PATH}
    COMMAND ${WIX_EXECUTABLE}
        build ${WIX_BUNDLE_OUTPUT}
        -ext WixToolset.BootstrapperApplications.wixext
        -ext WixToolset.Util.wixext
        -o ${QTNOTE_INSTALLER_EXE_PATH}
    DEPENDS ${WIX_BUNDLE_OUTPUT} ${QTNOTE_MSI} ${LICENSE_RTF}
    COMMENT "Building Burn Bootstrapper"
)

add_custom_target(burn_installer DEPENDS ${QTNOTE_INSTALLER_EXE_PATH} ${LICENSE_TXT} ${WIX_TEMPLATE} ${WIX_BUNDLE_TEMPLATE})
set_target_properties(burn_installer PROPERTIES EXCLUDE_FROM_ALL TRUE)
