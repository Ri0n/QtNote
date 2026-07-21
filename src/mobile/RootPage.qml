import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    signal openPluginSettings(string pluginId, string pluginName)
    signal openStorageSettings(string storageId, string storageName)

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 0

            Item {
                Layout.preferredWidth: addButton.implicitWidth
                Layout.fillHeight: true
            }

            Label {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: tabs.currentIndex === 0 ? qsTr("Notes")
                     : tabs.currentIndex === 1 ? qsTr("Plugins")
                     : tabs.currentIndex === 2 ? qsTr("Storages")
                     : qsTr("Settings")
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 20
                font.bold: true
            }

            ToolButton {
                id: addButton
                Layout.fillHeight: true
                visible: tabs.currentIndex === 0
                text: "+"
                font.pixelSize: 28
                Accessible.name: qsTr("Add note")
                onClicked: notesPage.createNote()
            }
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabs.currentIndex

        NotesPage {
            id: notesPage
        }
        PluginsPage {
            onOpenSettings: (pluginId, pluginName) => root.openPluginSettings(pluginId, pluginName)
        }
        StoragesPage {
            onOpenSettings: (storageId, storageName) => root.openStorageSettings(storageId, storageName)
        }
        AppSettingsPage { }
    }

    footer: TabBar {
        id: tabs

        TabButton { text: qsTr("Notes") }
        TabButton { text: qsTr("Plugins") }
        TabButton { text: qsTr("Storages") }
        TabButton { text: qsTr("Settings") }
    }
}
