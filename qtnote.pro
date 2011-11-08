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
RESOURCES += main.qrc
QT += xml

include (src/src.pri)

TRANSLATIONS += langs/qtnote_en.ts langs/qtnote_ru.ts
unix {
	isEmpty(PREFIX) {
		PREFIX = /usr
	}
	DATADIR = $$PREFIX/share

    target.path = $$PREFIX/bin
    INSTALLS += target
    
    # translations
	TRANSLATIONSDIR = $$DATADIR/$$TARGET
	DEFINES += TRANSLATIONSDIR=\\\"$$TRANSLATIONSDIR\\\" \
		DATADIR=\\\"$$DATADIR\\\" \
		APPNAME=\\\"$$TARGET\\\"
    
	LANGS = en ru
	for(t, LANGS):translations.files += "langs/qtnote_$${t}.qm"
	translations.path = $$TRANSLATIONSDIR
	DISTFILES += $$translations

	# Desktop file
	desktop.files = $${TARGET}.desktop
	desktop.path = $$DATADIR/applications

	# Desktop pixmap
	pixmap.files = images/$${TARGET}.png
	pixmap.path = $$DATADIR/pixmaps

	INSTALLS += translations desktop pixmap
}
RC_FILE = win/$${TARGET}.rc

MOC_DIR = .moc
OBJECTS_DIR = .obj
RCC_DIR = .rcc
UI_DIR = .ui
