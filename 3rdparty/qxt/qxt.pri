SOURCES += \
	$$PWD/qxtglobalshortcut.cpp

mac:SOURCES += $$PWD/qxtglobalshortcut_mac.cpp
win:SOURCES += $$PWD/qxtglobalshortcut_win.cpp
unix:!mac:SOURCES += $$PWD/qxtglobalshortcut_x11.cpp

HEADERS += \
	$$PWD/qxtglobalshortcut.h \
	$$PWD/qxtglobalshortcut_p.h
