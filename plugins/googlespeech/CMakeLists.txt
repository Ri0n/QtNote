include(QtNoteMacro)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Network REQUIRED)

add_qtnote_plugin(googlespeech "Google Cloud Speech-to-Text backend" ON unix macosx windows SOURCES
    googlespeechplugin.cpp
    googlespeechplugin.h
)

target_link_libraries(googlespeech
    ${EXTRA_LINK_TARGET}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

install_qtnote_plugin(googlespeech)
