find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

add_executable(filedraftstore_test filedraftstore_test.cpp)
target_link_libraries(filedraftstore_test PRIVATE qtnote Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME filedraftstore_test COMMAND filedraftstore_test)

add_executable(secureenvelope_test secureenvelope_test.cpp)
target_link_libraries(secureenvelope_test PRIVATE qtnote Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME secureenvelope_test COMMAND secureenvelope_test)

add_executable(xmppnotecodec_test
    xmppnotecodec_test.cpp
    ../plugins/xmpppubsub/xmppnotecodec.cpp
)
target_include_directories(xmppnotecodec_test PRIVATE ../plugins/xmpppubsub)
target_link_libraries(xmppnotecodec_test PRIVATE qtnote Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME xmppnotecodec_test COMMAND xmppnotecodec_test)

find_package(QXmppQt${QT_VERSION_MAJOR} 1.11 REQUIRED)
add_executable(xmppkeysyncextension_test
    xmppkeysyncextension_test.cpp
    ../plugins/xmpppubsub/xmppkeysyncextension.cpp
)
target_include_directories(xmppkeysyncextension_test PRIVATE ../plugins/xmpppubsub)
target_link_libraries(xmppkeysyncextension_test PRIVATE Qt${QT_VERSION_MAJOR}::Test QXmpp::QXmpp)
add_test(NAME xmppkeysyncextension_test COMMAND xmppkeysyncextension_test)

add_executable(xmppomemopubsubitems_test
    xmppomemopubsubitems_test.cpp
    ../plugins/xmpppubsub/xmppomemopubsubitems.cpp
)
target_include_directories(xmppomemopubsubitems_test PRIVATE ../plugins/xmpppubsub)
target_link_libraries(xmppomemopubsubitems_test PRIVATE Qt${QT_VERSION_MAJOR}::Test QXmpp::QXmpp)
add_test(NAME xmppomemopubsubitems_test COMMAND xmppomemopubsubitems_test)

add_executable(xmpppersistenttruststorage_test
    xmpppersistenttruststorage_test.cpp
    ../plugins/xmpppubsub/xmpppersistenttruststorage.cpp
)
target_include_directories(xmpppersistenttruststorage_test PRIVATE ../plugins/xmpppubsub)
target_link_libraries(xmpppersistenttruststorage_test PRIVATE qtnote Qt${QT_VERSION_MAJOR}::Test QXmpp::QXmpp)
add_test(NAME xmpppersistenttruststorage_test COMMAND xmpppersistenttruststorage_test)

find_package(QXmppOmemoQt${QT_VERSION_MAJOR} 1.11 REQUIRED)
add_executable(xmppomemostorage_test
    xmppomemostorage_test.cpp
    ../plugins/xmpppubsub/xmppomemostorage.cpp
)
target_include_directories(xmppomemostorage_test PRIVATE ../plugins/xmpppubsub)
target_link_libraries(xmppomemostorage_test PRIVATE qtnote Qt${QT_VERSION_MAJOR}::Test QXmpp::Omemo)
add_test(NAME xmppomemostorage_test COMMAND xmppomemostorage_test)
