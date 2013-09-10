# -------------------------------------------------
# Project created by QtCreator 2009-02-10T13:06:32
# QtNote - Simple note-taking application
# Copyright (C) 2010 Ili'nykh Sergey
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

include(../common.pri)

TARGET = $$APPNAME
TEMPLATE = app
CONFIG += tomboy # force tomboy
RESOURCES += ../main.qrc

QT += xml
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= printsupport
    QT *= widgets
    greaterThan(QT_MAJOR_VERSION, 5)|greaterThan(QT_MINOR_VERSION, 0) {
	unix:!mac:QT += x11extras
    }
}

unix {
    target.path = $$PREFIX/bin
    INSTALLS += target
}

RC_FILE = win/$${TARGET}.rc

MOC_DIR = .moc
OBJECTS_DIR = .obj
RCC_DIR = .rcc
UI_DIR = .ui


SOURCES += $$PWD/main.cpp \
	$$PWD/notedialog.cpp \
	$$PWD/note.cpp \
	$$PWD/notemanager.cpp \
	$$PWD/notestorage.cpp \
	$$PWD/ptfstorage.cpp \
	$$PWD/ptfdata.cpp \
	$$PWD/notedata.cpp \
	$$PWD/filenotedata.cpp \
	$$PWD/aboutdlg.cpp \
	$$PWD/optionsdlg.cpp \
	$$PWD/filestorage.cpp \
	$$PWD/utils.cpp \
	$$PWD/notesmodel.cpp \
	$$PWD/notemanagerdlg.cpp \
	$$PWD/notemanagerview.cpp \
	$$PWD/notedialogedit.cpp \
	$$PWD/ptfstoragesettingswidget.cpp \
	$$PWD/qtnote.cpp \
	$$PWD/typeaheadfind.cpp \
	$$PWD/shortcutedit.cpp \
	$$PWD/shortcutsmanager.cpp \
	$$PWD/notewidget.cpp \
    pluginmanager.cpp

HEADERS += $$PWD/notedialog.h \
	$$PWD/note.h \
	$$PWD/notemanager.h \
	$$PWD/notestorage.h \
	$$PWD/ptfstorage.h \
	$$PWD/ptfdata.h \
	$$PWD/notedata.h \
	$$PWD/filenotedata.h \
	$$PWD/aboutdlg.h \
	$$PWD/optionsdlg.h \
	$$PWD/filestorage.h \
	$$PWD/utils.h \
	$$PWD/notesmodel.h \
	$$PWD/notemanagerdlg.h \
	$$PWD/notemanagerview.h \
	$$PWD/notedialogedit.h \
	$$PWD/ptfstoragesettingswidget.h \
	$$PWD/qtnote.h \
	$$PWD/typeaheadfind.h \
	$$PWD/shortcutedit.h \
	$$PWD/shortcutsmanager.h \
	$$PWD/notewidget.h \
    pluginmanager.h

tomboy { 
    SOURCES += $$PWD/tomboystorage.cpp \
        $$PWD/tomboydata.cpp
    HEADERS += $$PWD/tomboystorage.h \
        $$PWD/tomboydata.h
    DEFINES += TOMBOY
}

FORMS += $$PWD/notedialog.ui \
	$$PWD/aboutdlg.ui \
	$$PWD/optionsdlg.ui \
	$$PWD/notemanagerdlg.ui \
	$$PWD/ptfstoragesettingswidget.ui \
	$$PWD/notewidget.ui

INCLUDEPATH += $$PWD

# required for shortcuts
unix:!macx:LIBS += -L$$QMAKE_LIBDIR_X11 -lX11

macx {
    LIBS = -framework ApplicationServices
}

include(../3rdparty/qtsingleapplication/qtsingleapplication.pri)
