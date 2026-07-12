include(QtNoteMacro)

add_qtnote_plugin(
    xmpppubsub
    "Stores notes in a private persistent XMPP PEP node"
    ON
    unix macosx windows
    SOURCES
        xmppdto.h
        qtnotepubsubitem.cpp
        qtnotepubsubitem.h
        xmpppepextension.cpp
        xmpppepextension.h
        xmppkeysyncextension.cpp
        xmppkeysyncextension.h
        xmppkeyresolutiondialog.cpp
        xmppkeyresolutiondialog.h
        xmppnotecodec.cpp
        xmppnotecodec.h
        xmppomemostorage.cpp
        xmppomemostorage.h
        xmppworker.cpp
        xmppworker.h
        xmppsettingswidget.cpp
        xmppsettingswidget.h
        xmppstorage.cpp
        xmppstorage.h
        xmppplugin.cpp
        xmppplugin.h
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Network Xml REQUIRED)
find_package(QXmppQt${QT_VERSION_MAJOR} 1.11 REQUIRED)
find_package(QXmppOmemoQt${QT_VERSION_MAJOR} 1.11 REQUIRED)

target_link_libraries(
    xmpppubsub
    ${EXTRA_LINK_TARGET}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Xml
    QXmpp::QXmpp
    QXmpp::Omemo
)

install_qtnote_plugin(xmpppubsub)
