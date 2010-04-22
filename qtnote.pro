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
TARGET = qtnote
TEMPLATE = app
CONFIG += tomboy # force tomboy
SOURCES += main.cpp \
    mainwidget.cpp \
    notedialog.cpp \
    note.cpp \
    notemanager.cpp \
    notestorage.cpp \
    ptfstorage.cpp \
    ptfdata.cpp \
    notedata.cpp \
    filenotedata.cpp \
    aboutdlg.cpp
HEADERS += mainwidget.h \
    notedialog.h \
    note.h \
    notemanager.h \
    notestorage.h \
    ptfstorage.h \
    ptfdata.h \
    notedata.h \
    filenotedata.h \
    aboutdlg.h
tomboy { 
    SOURCES += tomboystorage.cpp \
        tomboydata.cpp
    HEADERS += tomboystorage.h \
        tomboydata.h
    DEFINES += TOMBOY
}
RESOURCES += main.qrc
QT += xml
FORMS += notedialog.ui \
    aboutdlg.ui
unix { 
	target.path = $$PREFIX/bin
    INSTALLS += target
}

TRANSLATIONS = langs/qtnote_ru.ts
CODECFORTR = UTF-8
