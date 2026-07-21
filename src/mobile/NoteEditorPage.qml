pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var editor
    signal backRequested()

    function checkpointEditor() {
        Qt.inputMethod.commit()
        blockEditor.flushPendingEditorChanges()
        return mobileApp.saveCurrentNote()
    }

    function closeEditor() {
        saveTimer.stop()
        checkpointEditor()
        if (mobileApp.closeCurrentNote())
            backRequested()
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 0

            ToolButton {
                Layout.fillHeight: true
                text: qsTr("<")
                font.pixelSize: 30
                Accessible.name: qsTr("Back")
                onClicked: root.closeEditor()
            }

            Label {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: editor && editor.text.length > 0 ? editor.text.split("\n")[0] : qsTr("New note")
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 20
                font.bold: true
            }

            ToolButton {
                Layout.fillHeight: true
                text: qsTr("MD")
                enabled: false
                Accessible.name: qsTr("Markdown")
            }
        }
    }

    NoteBlockEditor {
        id: blockEditor
        anchors.fill: parent
        blockModel: root.editor ? root.editor.blockModel : null
        editorBackend: root.editor
        platformBackend: null
        onCountChanged: saveTimer.restart()

        Connections {
            target: blockEditor.blockModel
            function onContentsChanged() {
                saveTimer.restart()
            }
        }

        Component.onCompleted: focusInitialEditor()
    }

    Timer {
        id: saveTimer
        interval: 1000
        repeat: false
        onTriggered: root.checkpointEditor()
    }
}
