# -------------------------------------------------
# Project created by QtCreator 2009-02-10T13:06:32
# -------------------------------------------------
TARGET = qtnote
TEMPLATE = app
SOURCES += main.cpp \
	mainwidget.cpp \
    tomboynote.cpp \
    notedialog.cpp
HEADERS += mainwidget.h \
    tomboynote.h \
    notedialog.h
RESOURCES += main.qrc
QT += xml
FORMS += notedialog.ui

unix {
	target.path = $$PREFIX/usr/bin
	INSTALLS += target
}
