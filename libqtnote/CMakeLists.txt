cmake_minimum_required(VERSION 3.10.0)
include(QtNoteMacro)
project(libqtnote VERSION ${QTNOTE_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(qtnote SHARED
    aboutdlg.cpp
    aboutdlg.h
    colorbutton.cpp
    colorbutton.h
    defaults.h
    filenameprovider.cpp
    filenameprovider.h
    filenotedata.cpp
    filenotedata.h
    filestorage.cpp
    filestorage.h
    filestoragesettingswidget.cpp
    filestoragesettingswidget.h
    highlighterext.h
    humanfilenameprovider.cpp
    humanfilenameprovider.h
    note.cpp
    notedata.cpp
    notedata.h
    notedialog.cpp
    notedialog.h
    noteedit.cpp
    noteedit.h
    note.h
    notehighlighter.cpp
    notehighlighter.h
    notemanager.cpp
    notemanagerdlg.cpp
    notemanagerdlg.h
    notemanager.h
    notemanagerview.cpp
    notemanagerview.h
    notesmodel.cpp
    notesmodel.h
    notessearchmodel.cpp
    notessearchmodel.h
    notestorage.cpp
    notestorage.h
    notewidget.cpp
    notewidget.h
    optionsdlg.cpp
    optionsdlg.h
    optionsplugins.cpp
    optionsplugins.h
    pluginhost.cpp
    pluginhost.h
    pluginhostinterface.h
    pluginmanager.cpp
    pluginmanager.h
    ptfdata.cpp
    ptfdata.h
    ptfstorage.cpp
    ptfstorage.h
    qtnote.cpp
    qtnote_export.h
    qtnote.h
    shortcutedit.cpp
    shortcutedit.h
    shortcutsmanager.cpp
    shortcutsmanager.h
    typeaheadfind.cpp
    typeaheadfind.h
    utils.cpp
    utils.h
    uuidfilenameprovider.cpp
    uuidfilenameprovider.h
    aboutdlg.ui
    filestoragesettingswidget.ui
    notedialog.ui
    notemanagerdlg.ui
    notewidget.ui
    optionsdlg.ui
    optionsplugins.ui
    main.qrc
)

target_include_directories(qtnote PRIVATE "${CMAKE_SOURCE_DIR}/plugins" "${CMAKE_BINARY_DIR}")
target_include_directories(qtnote PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/qtnote>  # <prefix>/include/qnote
    )
target_link_libraries(qtnote PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport)
