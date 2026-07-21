pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    function createNote() {
        return mobileApp.createNote()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        TextField {
            Layout.fillWidth: true
            placeholderText: qsTr("Search notes")
        }

        ListView {
            id: notesView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: mobileApp.notesModel

            delegate: ItemDelegate {
                required property string title
                width: notesView.width
                text: title
            }

            Label {
                anchors.centerIn: parent
                width: Math.min(parent.width - 32, 340)
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: notesView.count === 0
                text: qsTr("No notes yet.")
                color: palette.mid
            }
        }
    }
}
