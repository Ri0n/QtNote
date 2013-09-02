### Version handling ###

QT += core

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
