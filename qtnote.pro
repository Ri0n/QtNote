# -------------------------------------------------
# Project created by QtCreator 2009-02-10T13:06:32
# -------------------------------------------------
TARGET = qtnote
TEMPLATE = app
SOURCES += main.cpp \
    mainwidget.cpp \
    tomboynote.cpp \
    notedialog.cpp \
    note.cpp \
    notemanager.cpp \
    notestorage.cpp \
    tomboystorage.cpp \
    qtnoteptfstorage.cpp
HEADERS += mainwidget.h \
    tomboynote.h \
    notedialog.h \
    note.h \
    notemanager.h \
    notestorage.h \
    tomboystorage.h \
    qtnoteptfstorage.h
RESOURCES += main.qrc
QT += xml
FORMS += notedialog.ui
unix { 
    target.path = $$PREFIX/usr/bin
    INSTALLS += target
}
