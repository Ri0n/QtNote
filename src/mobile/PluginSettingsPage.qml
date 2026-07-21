import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: page

    required property string pluginId
    required property string pluginName
    signal backRequested()

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: qsTr("Back")
                onClicked: page.backRequested()
            }
            Label {
                Layout.fillWidth: true
                text: page.pluginName
                font.pixelSize: 20
                font.bold: true
                elide: Text.ElideRight
            }
        }
    }

    Label {
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 340)
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: qsTr("This plugin does not expose mobile settings yet.")
        color: palette.mid
    }
}
