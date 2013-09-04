include(../common.pri)

TEMPLATE = subdirs

exists(/usr/include/KDE):!nokde:SUBDIRS += kdeintegration

!nocrypt:SUBDIRS += crypt
