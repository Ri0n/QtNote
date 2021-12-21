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
target_link_libraries(macosx ${EXTRA_LINK_TARGET} Qt5::Core Qt5::Gui Qt5::Widgets)

install(TARGETS macosx DESTINATION ${PLUGINSDIR})
