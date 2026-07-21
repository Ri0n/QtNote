pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: page

    signal openSettings(string storageId, string storageName)

    ListView {
        id: storagesView
        anchors.fill: parent
        anchors.margins: 12
        clip: true
        model: mobileApp.storagesModel

        delegate: ItemDelegate {
            id: storageDelegate

            required property string storageId
            required property string name
            required property bool accessible
            required property bool configurable
            required property string tooltip

            width: storagesView.width
            enabled: accessible
            rightPadding: configurable ? settingsButton.width + 24 : 12
            onClicked: {
                if (configurable)
                    page.openSettings(storageId, name)
            }

            contentItem: ColumnLayout {
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: storageDelegate.name
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: storageDelegate.accessible ? storageDelegate.tooltip : qsTr("Not accessible")
                    visible: text.length > 0
                    elide: Text.ElideRight
                    color: palette.mid
                    font.pixelSize: 13
                }
            }

            ToolButton {
                id: settingsButton

                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                visible: storageDelegate.configurable
                text: qsTr("Settings")
                onClicked: page.openSettings(storageDelegate.storageId, storageDelegate.name)
            }
        }

        Label {
            anchors.centerIn: parent
            width: Math.min(parent.width - 32, 340)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: storagesView.count === 0
            text: qsTr("Mobile storages are not connected yet.")
            color: palette.mid
        }
    }
}
