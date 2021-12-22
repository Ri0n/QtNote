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
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
include(QtNoteMacro)
message(STATUS "Version: QtNote-${QTNOTE_VERSION}")
project(qtnote VERSION ${QTNOTE_VERSION})

option(QTNOTE_DEVEL "Enable development mode (adjusts some paths and output)" OFF)

find_package(Qt5 COMPONENTS Widgets Core Gui PrintSupport REQUIRED)
if (Qt5Widgets_VERSION VERSION_LESS 5.10.0)
    message(FATAL_ERROR "Minimum supported Qt5 version is 5.10!")
endif()

set(CMAKE_CXX_STANDARD 17)

if(UNIX AND NOT (APPLE OR HAIKU))
    set(UNIXLIKE ON)
endif()

include(GNUInstallDirs)
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
