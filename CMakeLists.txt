# -------------------------------------------------
# QtNote - Simple note-taking application
# Copyright (C) 2020 Sergei Ilinykh
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# Contacts:
# E-Mail: rion4ik@gmail.com XMPP: rion@jabber.ru
# -------------------------------------------------

cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
include(QtNoteMacro)
message(STATUS "Version: QtNote-${QTNOTE_VERSION}")
project(qtnote VERSION ${QTNOTE_VERSION} LANGUAGES CXX)

option(QTNOTE_DEVEL "Enable development mode (adjusts some paths and output)" OFF)

if("${QT_DEFAULT_MAJOR_VERSION}" STREQUAL "")
    if(WIN32 OR APPLE)
        set(QT_DEFAULT_MAJOR_VERSION 6)
    else()
        set(QT_DEFAULT_MAJOR_VERSION 5)
    endif()
endif()

find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS Widgets Core Gui PrintSupport REQUIRED)
if (Qt${QT_DEFAULT_MAJOR_VERSION}Widgets_VERSION VERSION_LESS 5.10.0)
    message(FATAL_ERROR "Minimum supported Qt5 version is 5.10!")
endif()
set(QT_VERSION_MAJOR ${Qt${QT_DEFAULT_MAJOR_VERSION}Core_VERSION_MAJOR})

if(UNIX AND NOT (APPLE OR HAIKU))
    set(UNIXLIKE ON)
endif()

set(I18N_TRANSLATED_LANGUAGES)
file(GLOB qtnote_TRS "${CMAKE_CURRENT_SOURCE_DIR}/qtnote_*.ts")
foreach(_LANG ${qtnote_TRS})
    get_filename_component(_SHORT_LANG ${_LANG} NAME_WE)
    string(REPLACE "@" "_" _SHORT_LANG ${_SHORT_LANG})
    list(APPEND I18N_TRANSLATED_LANGUAGES ${_SHORT_LANG})
endforeach()
if(Qt${QT_VERSION_MAJOR}Core_VERSION VERSION_LESS 6.3.0)
    include(GNUInstallDirs)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    if(WIN32 AND CMAKE_RUNTIME_OUTPUT_DIRECTORY STREQUAL "")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    set(USE_FOLDERS ON)
elseif(Qt${QT_VERSION_MAJOR}Core_VERSION VERSION_LESS 6.7.0)
    qt_standard_project_setup()
else()
    qt_standard_project_setup(
        I18N_TRANSLATED_LANGUAGES ${I18N_TRANSLATED_LANGUAGES}
        I18N_SOURCE_LANGUAGE en
    )
endif()
set(APP_NAME qtnote)
set(TRANSLATIONSDIR ${CMAKE_INSTALL_FULL_DATADIR}/${APP_NAME})
set(PLUGINSDIR ${CMAKE_INSTALL_FULL_LIBDIR}/${APP_NAME})
configure_file(qtnote_config.h.in qtnote_config.h)

if(QTNOTE_DEVEL)
    add_definitions(-DQTNOTE_DEVEL)
endif()

if(WIN32)
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/qtnote)
endif()

add_subdirectory(3rdparty)
add_subdirectory(libqtnote)
add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(langs)
