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
        xmppworker.cpp
        xmppworker.h
        xmppdata.cpp
        xmppdata.h
        xmppsettingswidget.cpp
        xmppsettingswidget.h
        xmppstorage.cpp
        xmppstorage.h
        xmppplugin.cpp
        xmppplugin.h
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Network Xml REQUIRED)
find_package(QXmppQt${QT_VERSION_MAJOR} 1.11 REQUIRED)

target_link_libraries(
    xmpppubsub
    ${EXTRA_LINK_TARGET}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Xml
    QXmpp::QXmpp
)

install_qtnote_plugin(xmpppubsub)
