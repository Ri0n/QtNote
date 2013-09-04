### Version handling ###

QT *= core

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
DEFINES += QTNOTE_VERSION=$$VERSION
DEFINES += QTNOTE_VERSION_W=$$join(VERSIONS_FULL, ",")
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
