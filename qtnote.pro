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
#VERSION = 0.2.3.1
QT += xml
greaterThan(QT_MAJOR_VERSION, 4) {
    QT *= printsupport
    greaterThan(QT_MAJOR_VERSION, 5)|greaterThan(QT_MINOR_VERSION, 0) {
        unix:!mac:QT += x11extras
    }
}

### Version handling ###
isEmpty(VERSION) {
	VERSION = $$cat($$PWD/version)
}
VERSIONS = $$split(VERSION, ".")
VERSION_MAJ = $$member(VERSIONS, 0)

VERPOS = 0 1 2 3
for(i, VERPOS) {
	verval = $$member(VERSIONS, $${i})
	isEmpty(verval):VERSIONS_FULL += "0"
	!isEmpty(verval):VERSIONS_FULL += $${verval}
}
DEFINES += QTNOTE_VERSION=$$VERSION
DEFINES += QTNOTE_VERSION_W=$$join(VERSIONS_FULL, ",")
### End of version handling ###

include (src/src.pri)

TRANSLATIONS += \
    langs/qtnote_en.ts \
    langs/qtnote_fr.ts \
    langs/qtnote_ru.ts \
    langs/qtnote_vi.ts

unix {
    isEmpty(PREFIX) {
            PREFIX = /usr
    }
    isEmpty(DATADIR):DATADIR = $$PREFIX/share
    isEmpty(MANDIR):MANDIR = $$DATADIR/man/man1

    target.path = $$PREFIX/bin
    INSTALLS += target
    
    # translations
    TRANSLATIONSDIR = $$DATADIR/$$TARGET
    DEFINES += TRANSLATIONSDIR=\\\"$$TRANSLATIONSDIR\\\" \
            DATADIR=\\\"$$DATADIR\\\"

    LANGS = en fr ru vi
    for(t, LANGS):translations.files += "langs/qtnote_$${t}.qm"
    translations.path = $$TRANSLATIONSDIR
    DISTFILES += $$translations

    # Desktop file
    desktop.files = $${TARGET}.desktop
    desktop.path = $$DATADIR/applications

    # Desktop pixmap
    pixsizes = 16 22 32 48 64
    for(size, pixsizes) {
	path = $$DATADIR/icons/hicolor/$${size}x$${size}/apps
	extra = cp -f images/$${TARGET}$${size}.png $(INSTALL_ROOT)$$path/$${TARGET}.png
	eval(pix$${size}.path = $$path)
	eval(pix$${size}.extra = $$extra)
	eval(pixmaps += pix$${size})
    }

    pixmap_svg.path = $$DATADIR/icons/hicolor/scalable/apps
    pixmap_svg.files = images/$${TARGET}.svg

    # Man page
    man.files = docs/qtnote.1
    man.path = $$MANDIR

    INSTALLS += translations desktop $$pixmaps pixmap_svg man
}

DEFINES += APPNAME=\\\"$$TARGET\\\"
RC_FILE = win/$${TARGET}.rc

MOC_DIR = .moc
OBJECTS_DIR = .obj
RCC_DIR = .rcc
UI_DIR = .ui
