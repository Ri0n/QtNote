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

    header: EditorToolbar {
        editorBackend: root.editor
        blockEditor: blockEditor
        platformBackend: null
        compact: true
        showBackButton: true
        onBackRequested: root.closeEditor()
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
