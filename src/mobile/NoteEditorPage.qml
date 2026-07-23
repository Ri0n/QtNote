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

    function deleteEditor() {
        saveTimer.stop()
        Qt.inputMethod.commit()
        blockEditor.flushPendingEditorChanges()
        if (mobileApp.workspace.deleteNote(root.editor.storageId, root.editor.noteId))
            root.backRequested()
    }

    function requestDelete() {
        if (mobileApp.askBeforeDelete)
            deleteDialog.open()
        else
            deleteEditor()
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
        showDeleteButton: true
        onBackRequested: root.closeEditor()
        onDeleteRequested: root.requestDelete()
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

    Dialog {
        id: deleteDialog
        parent: root
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        modal: true
        title: qsTr("Delete note")
        standardButtons: Dialog.Yes | Dialog.No

        Label {
            width: Math.min(360, root.width - 48)
            wrapMode: Text.WordWrap
            text: qsTr("Delete this note?")
        }

        onAccepted: root.deleteEditor()
    }

    Timer {
        id: saveTimer
        interval: 1000
        repeat: false
        onTriggered: root.checkpointEditor()
    }
}
