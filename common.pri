### Version handling ###

QT *= core

greaterThan(QT_MAJOR_VERSION, 4) {
	CONFIG += c++11
} else {
	QMAKE_CXXFLAGS += -std=c++11
}
APPNAME = qtnote
DEFINES += APPNAME=\\\"$$APPNAME\\\"

### Start of version handling ###
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

DEFINES += QTNOTE_VERSION_S=$$VERSION
DEFINES += QTNOTE_VERSION_W=$$join(VERSIONS_FULL, ",")
DEFINES += QTNOTE_VERSION_PRODUCT=$$member(VERSIONS_FULL, 0)
DEFINES += QTNOTE_VERSION_MAJOR=$$member(VERSIONS_FULL, 1)
DEFINES += QTNOTE_VERSION_MINOR=$$member(VERSIONS_FULL, 2)
DEFINES += QTNOTE_VERSION_PATCH=$$member(VERSIONS_FULL, 3)
### End of version handling ###

unix {
	isEmpty(PREFIX) {
		PREFIX = /usr
	}
	isEmpty(DATADIR):DATADIR = $$PREFIX/share
	isEmpty(LIBDIR):LIBDIR = $$PREFIX/lib
	isEmpty(MANDIR):MANDIR = $$DATADIR/man/man1
	isEmpty(TRANSLATIONSDIR):TRANSLATIONSDIR = $$DATADIR/$$APPNAME

	DEFINES += LIBDIR=\\\"$$LIBDIR\\\" \
		DATADIR=\\\"$$DATADIR\\\" \
		TRANSLATIONSDIR=\\\"$$TRANSLATIONSDIR\\\"
}

devel {
	DEFINES += DEVEL
}

CONFIG += precompile_header
PRECOMPILED_HEADER = $$PWD/config.h

HEADERS += \
	$$PWD/plugins/trayimpl.h
