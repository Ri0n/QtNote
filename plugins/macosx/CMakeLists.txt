include(QtNoteMacro)

add_qtnote_plugin(macosx "Mac OS X integration" ON macosx)

add_library(macosx ${LIB_TYPE}
    ${QTNOTE_COMMON_PLUGIN_SRC}
    macosx.cpp
    macosx.h
    macosx.qrc
    macosxtray.cpp
    macosxtray.h
    )
target_link_libraries(macosx ${EXTRA_LINK_TARGET} Qt${QT_DEFAULT_MAJOR_VERSION}::Core Qt${QT_DEFAULT_MAJOR_VERSION}::Gui Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets)

install(TARGETS macosx LIBRARY DESTINATION ${PLUGINSDIR} COMPONENT Libraries NAMELINK_COMPONENT Development)
