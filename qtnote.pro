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

TEMPLATE = subdirs
SUBDIRS += 3rdparty libqtnote src plugins
CONFIG += ordered

TRANSLATIONS += \
	langs/qtnote_en.ts \
	langs/qtnote_fr.ts \
	langs/qtnote_ru.ts \
	langs/qtnote_vi.ts

include(common.pri)

unix {
	LANGS = en fr ru vi
	for(t, LANGS):translations.files += "langs/qtnote_$${t}.qm"
	translations.path = $$TRANSLATIONSDIR
	DISTFILES += $$translations

	# Desktop file
	desktop.files = $${TARGET}.desktop
	desktop.path = $$DATADIR/applications

	# Desktop pixmap
	pixsizes = 16 22 24 32 48 64
	for(size, pixsizes) {
	path = $$DATADIR/icons/hicolor/$${size}x$${size}/apps
	extra = cp -f libqtnote/images/$${TARGET}$${size}.png $(INSTALL_ROOT)$$path/$${TARGET}.png
	eval(pix$${size}.path = $$path)
	eval(pix$${size}.extra = $$extra)
	eval(pixmaps += pix$${size})
	}

	pixmap_svg.path = $$DATADIR/icons/hicolor/scalable/apps
	pixmap_svg.files = libqtnote/images/$${TARGET}.svg

	# Man page
	man.files = docs/qtnote.1
	man.path = $$MANDIR

	INSTALLS += translations desktop $$pixmaps pixmap_svg man
}
