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
SOURCES += $$PWD/main.cpp \
    $$PWD/mainwidget.cpp \
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
	$$PWD/notemanagermodel.cpp \
    src/notemanagerdlg.cpp
HEADERS += $$PWD/mainwidget.h \
    $$PWD/notedialog.h \
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
	$$PWD/notemanagermodel.h \
    src/notemanagerdlg.h
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
    src/notemanagerdlg.ui
