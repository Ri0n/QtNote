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

libqtnote.depends = 3rdparty
src.depends = libqtnote
plugins.depends = libqtnote

TRANSLATIONS += \
    langs/qtnote_en.ts \
    langs/qtnote_fr.ts \
    langs/qtnote_ru.ts \
    langs/qtnote_vi.ts

include(common.pri)

LANGS = en fr ru vi
for(t, LANGS):translations.files += "langs/qtnote_$${t}.qm"

unix {
    translations.path = $$TRANSLATIONSDIR
    DISTFILES += $$translations

    # Desktop file
    desktop.files = $${TARGET}.desktop
    desktop.path = $$DATADIR/applications

    # Desktop pixmap
    pixsizes = 16 22 24 32 48 64
    for(size, pixsizes) {
    path = $$DATADIR/icons/hicolor/$${size}x$${size}/apps
    extra = cp -f $${PWD}/libqtnote/images/$${TARGET}$${size}.png $(INSTALL_ROOT)$$path/$${TARGET}.png
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

    commonconfig.input = install_common.pri.in
    commonconfig.output = tmp/common.pri
    QMAKE_SUBSTITUTES += commonconfig

    cc_inst.path = $$DATADIR/$$APPNAME
    cc_inst.files = tmp/common.pri
    INSTALLS += cc_inst
}

win32 {
    qtplatform.path = $$WININST_PREFIX/langs
    for(t, LANGS) {
        qttranslations.files += "$$[QT_INSTALL_TRANSLATIONS]/qt_$${t}.qm"
        qtmods = base script quick1 multimedia xmlpatterns
        for(mod, qtmods):qttranslations.files += "$$[QT_INSTALL_TRANSLATIONS]/qt$${mod}_$${t}.qm"
    }
    qttranslations.path = $$WININST_PREFIX/langs
    translations.path = $$WININST_PREFIX/langs
    INSTALLS += translations qttranslations

    qtlibs.path = $$WININST_PREFIX
    greaterThan(QT_MAJOR_VERSION, 4) {
        qtdlls = Qt5Core Qt5Gui Qt5Widgets Qt5Xml Qt5Network Qt5PrintSupport
    } else {
        qtdlls = QtCore4 QtGui4 QtXml4
    }
    for(lib, qtdlls) {
        qtlibs.files += $$[QT_INSTALL_BINS]/$${lib}.dll
    }

    win32-g++ {
        gccdlls = libgcc_s_dw2-1.dll libwinpthread-1.dll
        for(lib, gccdlls) {
            gccdll = $$system(g++ -print-file-name=$${lib})
            !isEmpty(gccdll):exists($${gccdll}):qtlibs.files += $${gccdll}
        }

        # workaround for QTBUG-16372 (still broken in qt-5.5.0)
        # move libstdc++-6.dll to the list above when fixed
        qtlibs.depends += copy_libstdcpp
        copy_libstdcpp.target = copy_libstdcpp
        gccdll = $$system(g++ -print-file-name=libstdc++-6.dll)
        copy_libstdcpp.commands = $(COPY) \"$$shell_path($${gccdll})\" \"$$shell_path($${qtlibs.path})\"
        QMAKE_EXTRA_TARGETS += copy_libstdcpp
        # end of workaround
    }

    qtplatform.path = $$WININST_PREFIX/platforms
    qtplugins = qminimal qoffscreen qwindows
    for(lib, qtplugins) {
        qtplatform.files += $$[QT_INSTALL_PLUGINS]/platforms/$${lib}.dll
    }
    qtprintsupport.path = $$WININST_PREFIX/printsupport
    qtprintsupport.files += $$[QT_INSTALL_PLUGINS]/printsupport/windowsprintersupport.dll

    INSTALLS += qtlibs qtplatform qtprintsupport
}

OTHER_FILES += version install_common.pri.in
