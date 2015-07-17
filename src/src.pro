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

unix {
    target.path = $$PREFIX/bin
    INSTALLS += target
}

win32 {
	target.path = $$WININST_PREFIX
	INSTALLS += target
}

RC_FILE = win/$${TARGET}.rc


SOURCES += $$PWD/main.cpp

HEADERS += 

LIBS += -L$$OUT_PWD/../libqtnote$$DBG_SUBDIR -lqtnote
INCLUDEPATH += $$PWD/../libqtnote
DEPENDPATH += $$PWD/../libqtnote
DEFINES += QTNOTE_IMPORT

win32:CONFIG += bundled_singleapp
else:!bundled_singleapp {
	dirs = $$[QMAKE_MKSPECS]
	dirs = $$split(dirs, ":")
	qsa_found = 0
	for(d, dirs):exists($${d}/features/qtsingleapplication.prf):qsa_found = 1
	contains(qsa_found,0) {
		warning( QtSingleApplication is not installed. Use bundled instead )
		CONFIG += bundled_singleapp
	}
}
bundled_singleapp:include(../3rdparty/qtsingleapplication/qtsingleapplication.pri)
else:CONFIG += qtsingleapplication
