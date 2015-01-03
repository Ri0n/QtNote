SOURCES += \
	$$PWD/qxtglobalshortcut.cpp


mac:SOURCES += $$PWD/qxtglobalshortcut_mac.cpp
win32:SOURCES += $$PWD/qxtglobalshortcut_win.cpp
unix:!macx:SOURCES += $$PWD/qxtglobalshortcut_x11.cpp

HEADERS += \
	$$PWD/qxtglobal.h \
	$$PWD/qxtglobalshortcut.h \
	$$PWD/qxtglobalshortcut_p.h

INCLUDEPATH += $$PWD
