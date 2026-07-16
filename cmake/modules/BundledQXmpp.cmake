include(ExternalProject)
include(FindPkgConfig)

if(NOT QT_VERSION_MAJOR EQUAL 6)
    message(FATAL_ERROR "Bundled QXmpp is supported only with Qt 6")
endif()
if(NOT UNIX)
    message(FATAL_ERROR "Bundled QXmpp currently supports Unix-like targets only")
endif()

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Network Xml)
find_package(OpenSSL 3.0 REQUIRED COMPONENTS Crypto)
pkg_check_modules(OmemoC REQUIRED IMPORTED_TARGET libomemo-c)

set(QTNOTE_BUNDLED_QXMPP_VERSION "1.15.1")
set(QTNOTE_QXMPP_SOURCE_DIR "" CACHE PATH "Local QXmpp source directory (avoids downloading the release tarball)")
option(QTNOTE_BUNDLED_QXMPP_STATIC "Link bundled QXmpp statically into its consumers" ON)

set(_qxmpp_prefix "${CMAKE_BINARY_DIR}/_deps/qxmpp")
set(_qxmpp_install_dir "${_qxmpp_prefix}/install")
set(_qxmpp_library_dir "${_qxmpp_install_dir}/${CMAKE_INSTALL_LIBDIR}")
set(_qxmpp_include_dir "${_qxmpp_install_dir}/${CMAKE_INSTALL_INCLUDEDIR}/QXmppQt6")
set(_qxmpp_omemo_include_dir "${_qxmpp_include_dir}/Omemo")

if(QTNOTE_BUNDLED_QXMPP_STATIC)
    set(_qxmpp_library_type STATIC)
    set(_qxmpp_library_suffix "${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(_qxmpp_build_shared OFF)
else()
    set(_qxmpp_library_type SHARED)
    set(_qxmpp_library_suffix "${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(_qxmpp_build_shared ON)
endif()
set(_qxmpp_library "${_qxmpp_library_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}QXmppQt6${_qxmpp_library_suffix}")
set(_qxmpp_omemo_library "${_qxmpp_library_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}QXmppOmemoQt6${_qxmpp_library_suffix}")

if(QTNOTE_QXMPP_SOURCE_DIR)
    if(NOT EXISTS "${QTNOTE_QXMPP_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "QTNOTE_QXMPP_SOURCE_DIR does not contain a QXmpp source tree: ${QTNOTE_QXMPP_SOURCE_DIR}")
    endif()
    set(_qxmpp_source_args
        SOURCE_DIR "${QTNOTE_QXMPP_SOURCE_DIR}"
        DOWNLOAD_COMMAND ""
        UPDATE_COMMAND ""
    )
else()
    set(_qxmpp_source_args
        URL "https://download.kde.org/unstable/qxmpp/qxmpp-${QTNOTE_BUNDLED_QXMPP_VERSION}.tar.xz"
        URL_HASH "SHA256=0747758a4f5b5ea4c60686c65b390766f1909d09e1a5a457c8e80ef272730c46"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
endif()

set(_qxmpp_cmake_args
    "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
    "-DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}"
    "-DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
    "-DBUILD_SHARED=${_qxmpp_build_shared}"
    "-DBUILD_TESTING=OFF"
    "-DBUILD_DOCUMENTATION=OFF"
    "-DBUILD_DOCBOOK=OFF"
    "-DBUILD_EXAMPLES=OFF"
    "-DBUILD_OMEMO=ON"
    "-DWITH_GSTREAMER=OFF"
    "-DWITH_ENCRYPTION=ON"
)
if(CMAKE_CXX_COMPILER)
    list(APPEND _qxmpp_cmake_args "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
endif()
if(CMAKE_CXX_COMPILER_LAUNCHER)
    list(APPEND _qxmpp_cmake_args "-DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}")
endif()

ExternalProject_Add(qtnote_bundled_qxmpp
    ${_qxmpp_source_args}
    PREFIX "${_qxmpp_prefix}"
    INSTALL_DIR "${_qxmpp_install_dir}"
    CMAKE_ARGS ${_qxmpp_cmake_args}
    BUILD_BYPRODUCTS
        "${_qxmpp_library}"
        "${_qxmpp_omemo_library}"
)

# Imported targets let the rest of QtNote use the same target names as a
# system QXmpp package. The directories must exist when CMake validates the
# imported target interfaces; ExternalProject fills them during the build.
file(MAKE_DIRECTORY "${_qxmpp_include_dir}" "${_qxmpp_omemo_include_dir}")

add_library(QXmpp::QXmpp ${_qxmpp_library_type} IMPORTED GLOBAL)
set_target_properties(QXmpp::QXmpp PROPERTIES
    IMPORTED_LOCATION "${_qxmpp_library}"
    INTERFACE_INCLUDE_DIRECTORIES "${_qxmpp_include_dir}"
    INTERFACE_LINK_LIBRARIES "Qt6::Core;Qt6::Network;Qt6::Xml;OpenSSL::Crypto"
)
add_dependencies(QXmpp::QXmpp qtnote_bundled_qxmpp)

add_library(QXmpp::Omemo ${_qxmpp_library_type} IMPORTED GLOBAL)
set_target_properties(QXmpp::Omemo PROPERTIES
    IMPORTED_LOCATION "${_qxmpp_omemo_library}"
    INTERFACE_INCLUDE_DIRECTORIES "${_qxmpp_omemo_include_dir}"
    INTERFACE_LINK_LIBRARIES "QXmpp::QXmpp;PkgConfig::OmemoC;OpenSSL::Crypto"
)
add_dependencies(QXmpp::Omemo qtnote_bundled_qxmpp)

if(NOT QTNOTE_BUNDLED_QXMPP_STATIC)
    install(
        DIRECTORY "${_qxmpp_library_dir}/"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT Libraries
        FILES_MATCHING
            PATTERN "${CMAKE_SHARED_LIBRARY_PREFIX}QXmppQt6${CMAKE_SHARED_LIBRARY_SUFFIX}*"
            PATTERN "${CMAKE_SHARED_LIBRARY_PREFIX}QXmppOmemoQt6${CMAKE_SHARED_LIBRARY_SUFFIX}*"
    )
endif()
