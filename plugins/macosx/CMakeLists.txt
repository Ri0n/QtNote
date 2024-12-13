include(QtNoteMacro)

add_qtnote_plugin(macosx "Mac OS X integration" ON macosx SOURCES
    macosx.cpp
    macosx.h
    macosx.qrc
    macosxtray.cpp
    macosxtray.h
)

target_link_libraries(macosx ${EXTRA_LINK_TARGET} Qt${QT_DEFAULT_MAJOR_VERSION}::Core Qt${QT_DEFAULT_MAJOR_VERSION}::Gui Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets)

install_qtnote_plugin(macosx)
