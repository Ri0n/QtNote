pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

Item {
    id: root

    required property var workspace
    property var platformBackend: null
    property bool embeddedEditor: true
    property bool showCreateButton: true
    property bool confirmDelete: true
    property bool compact: width < 760
    property real navigationWidth: 340
    property string selectedStorageId: ""
    property string selectedNoteId: ""
    property string selectedTitle: ""
    property bool editorFocusOwned: false

    function flushEditorChanges() {
        Qt.inputMethod.commit()
        if (editorPanel.visible && editorLoader.item)
            editorLoader.item.blockEditor.flushPendingEditorChanges()
    }

    function checkpointEditor() {
        flushEditorChanges()
        return workspace.saveCurrentNote()
    }

    function reloadEditor() {
        if (!workspace.currentEditor || workspace.currentEditor.dirty)
            return false
        return workspace.reloadCurrentNote()
    }

    function closeWorkspace() {
        flushEditorChanges()
        return workspace.closeCurrentNote()
    }

    function insertionRowAtPoint(x, y) {
        if (!embeddedEditor || !editorLoader.item)
            return -1
        const editor = editorLoader.item.blockEditor
        const point = editor.mapFromItem(root, x, y)
        if (point.x < 0 || point.y < 0 || point.x >= editor.width || point.y >= editor.height)
            return -1
        return editor.insertionRowAtPoint(point.x, point.y)
    }

    function selectNote(storageId, noteId, title) {
        if (workspace.currentEditor && !checkpointEditor())
            return false
        selectedStorageId = storageId
        selectedNoteId = noteId
        selectedTitle = title
        return workspace.openNote(storageId, noteId)
    }

    function createNote() {
        if (workspace.currentEditor && !checkpointEditor())
            return false
        return workspace.createNote(selectedStorageId)
    }

    function openStandalone(storageId, noteId) {
        if (workspace.currentEditor && !checkpointEditor())
            return false
        return workspace.openStandalone(storageId, noteId)
    }

    function requestDelete(storageId, noteId, title) {
        selectedStorageId = storageId
        selectedNoteId = noteId
        selectedTitle = title
        if (confirmDelete) {
            deleteDialog.open()
            return true
        }
        if (!workspace.currentEditor || checkpointEditor())
            return workspace.deleteNote(storageId, noteId)
        return false
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Pane {
            id: navigationPane
            SplitView.preferredWidth: root.embeddedEditor ? root.navigationWidth : root.width
            SplitView.minimumWidth: root.embeddedEditor ? 230 : 0
            SplitView.maximumWidth: root.embeddedEditor ? Math.max(520, root.width * 0.65) : root.width
            padding: 8
            onWidthChanged: {
                if (root.embeddedEditor && width >= SplitView.minimumWidth)
                    root.navigationWidth = width
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    TextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search notes")
                        text: root.workspace.searchText
                        onTextEdited: root.workspace.searchText = text
                    }

                    ToolButton {
                        text: root.workspace.searching ? qsTr("…") : qsTr("↻")
                        enabled: !root.workspace.busy
                        Accessible.name: qsTr("Refresh notes")
                        onClicked: root.workspace.refresh()
                    }

                    ToolButton {
                        visible: root.showCreateButton
                        text: qsTr("+")
                        Accessible.name: qsTr("New note")
                        onClicked: root.createNote()
                    }
                }

                CheckBox {
                    text: qsTr("Search in text")
                    checked: root.workspace.searchInBody
                    onToggled: root.workspace.searchInBody = checked
                }

                TreeView {
                    id: notesTree
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.workspace.notesModel
                    Component.onCompleted: Qt.callLater(function() { expandRecursively(-1, 1) })
                    selectionModel: ItemSelectionModel { model: notesTree.model }

                    delegate: ItemDelegate {
                        id: delegateRoot

                        required property int row
                        required property int column
                        required property int depth
                        required property bool expanded
                        required property bool hasChildren
                        required property bool isTreeNode
                        required property string storageId
                        required property string noteId
                        required property int itemType
                        required property string title
                        required property string preview
                        required property bool loading
                        required property string errorString
                        required property bool hasMore
                        required property int noteCount

                        implicitWidth: notesTree.width
                        width: notesTree.width
                        leftPadding: 10 + depth * 18 + (isTreeNode && hasChildren ? 18 : 0)
                        rightPadding: 8
                        hoverEnabled: true
                        text: itemType === 0
                              ? (loading ? qsTr("%1 — loading…").arg(title)
                                         : qsTr("%1 (%2)").arg(title).arg(noteCount))
                              : title
                        font.bold: itemType === 0
                        highlighted: itemType === 0
                                     ? root.selectedStorageId === storageId
                                       && root.selectedNoteId.length === 0
                                     : root.selectedStorageId === storageId
                                       && root.selectedNoteId === noteId
                        ToolTip.visible: hovered && (errorString.length > 0 || preview.length > 0)
                        ToolTip.text: errorString.length > 0 ? errorString : preview

                        indicator: Label {
                            visible: delegateRoot.isTreeNode && delegateRoot.hasChildren
                            x: 8 + delegateRoot.depth * 18
                            anchors.verticalCenter: parent.verticalCenter
                            text: delegateRoot.expanded ? "▾" : "▸"
                            color: delegateRoot.highlighted
                                   ? delegateRoot.palette.highlightedText
                                   : delegateRoot.palette.text
                        }

                        Drag.active: noteDrag.active
                        Drag.source: delegateRoot
                        Drag.keys: ["qtnote-note"]
                        Drag.hotSpot.x: width / 2
                        Drag.hotSpot.y: height / 2

                        DragHandler {
                            id: noteDrag
                            target: null
                            enabled: delegateRoot.itemType === 1
                        }

                        DropArea {
                            anchors.fill: parent
                            enabled: delegateRoot.itemType === 0
                            keys: ["qtnote-note"]
                            onEntered: function(drag) {
                                if (drag.source && drag.source.storageId !== delegateRoot.storageId)
                                    drag.accepted = true
                            }
                            onDropped: function(drop) {
                                if (!drop.source || drop.source.storageId === delegateRoot.storageId)
                                    return
                                if (root.workspace.currentEditor && !root.checkpointEditor())
                                    return
                                if (root.workspace.moveNote(drop.source.storageId,
                                                            drop.source.noteId,
                                                            delegateRoot.storageId)) {
                                    drop.acceptProposedAction()
                                }
                            }
                        }

                        TapHandler {
                            acceptedButtons: Qt.LeftButton
                            enabled: delegateRoot.itemType === 1 && root.embeddedEditor
                            onDoubleTapped: root.openStandalone(delegateRoot.storageId,
                                                                 delegateRoot.noteId)
                        }

                        onClicked: {
                            notesTree.selectionModel.setCurrentIndex(notesTree.index(row, column),
                                                                     ItemSelectionModel.ClearAndSelect)
                            if (itemType === 0) {
                                root.selectedStorageId = storageId
                                root.selectedNoteId = ""
                                root.selectedTitle = title
                                notesTree.toggleExpanded(row)
                            } else {
                                root.selectNote(storageId, noteId, title)
                            }
                        }

                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: {
                                if (delegateRoot.itemType !== 1)
                                    return
                                root.selectedStorageId = delegateRoot.storageId
                                root.selectedNoteId = delegateRoot.noteId
                                root.selectedTitle = delegateRoot.title
                                noteMenu.popup()
                            }
                        }

                        Menu {
                            id: noteMenu
                            MenuItem {
                                text: qsTr("Open")
                                onTriggered: root.selectNote(delegateRoot.storageId,
                                                            delegateRoot.noteId,
                                                            delegateRoot.title)
                            }
                            MenuItem {
                                visible: root.embeddedEditor
                                text: qsTr("Open in separate window")
                                onTriggered: root.openStandalone(delegateRoot.storageId,
                                                                   delegateRoot.noteId)
                            }
                            MenuItem {
                                text: qsTr("Move…")
                                onTriggered: {
                                    root.selectedStorageId = delegateRoot.storageId
                                    root.selectedNoteId = delegateRoot.noteId
                                    root.selectedTitle = delegateRoot.title
                                    moveDialog.open()
                                }
                            }
                            MenuSeparator { }
                            MenuItem {
                                text: qsTr("Delete")
                                onTriggered: root.requestDelete(delegateRoot.storageId,
                                                                  delegateRoot.noteId,
                                                                  delegateRoot.title)
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: root.workspace.noteCount === 0 && !root.workspace.busy
                        width: Math.min(parent.width - 32, 360)
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        color: palette.mid
                        text: qsTr("No notes yet. Create the first note with the + button.")
                    }

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: root.workspace.busy && root.workspace.noteCount === 0
                        visible: running
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.workspace.errorString.length > 0
                    text: root.workspace.errorString
                    color: palette.brightText
                    wrapMode: Text.WordWrap
                }
            }
        }

        Pane {
            id: editorPanel
            visible: root.embeddedEditor
            SplitView.fillWidth: true
            SplitView.minimumWidth: 320
            padding: 0

            Loader {
                id: editorLoader
                anchors.fill: parent
                active: root.workspace.currentEditor !== null

                sourceComponent: Component {
                    Item {
                        id: editorWorkspace
                        property alias blockEditor: editorView

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            EditorToolbar {
                                Layout.fillWidth: true
                                editorBackend: root.workspace.currentEditor
                                platformBackend: root.platformBackend
                                blockEditor: editorView
                            }

                            NoteBlockEditor {
                                id: editorView
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                blockModel: root.workspace.currentEditor.blockModel
                                editorBackend: root.workspace.currentEditor
                                platformBackend: root.platformBackend

                                Connections {
                                    target: editorView.blockModel
                                    function onContentsChanged() { saveTimer.restart() }
                                }
                            }
                        }
                    }
                }
            }

            BusyIndicator {
                anchors.centerIn: parent
                z: 10
                running: root.workspace.loading && root.workspace.currentEditor !== null
                visible: running
            }

            ColumnLayout {
                anchors.fill: parent
                visible: root.workspace.currentEditor === null
                spacing: 8

                Item { Layout.fillHeight: true }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.workspace.loading ? qsTr("Loading note…")
                                                 : qsTr("Select a note to edit")
                    color: palette.mid
                }
                BusyIndicator {
                    Layout.alignment: Qt.AlignHCenter
                    running: root.workspace.loading
                    visible: running
                }
                Item { Layout.fillHeight: true }
            }
        }
    }

    Timer {
        id: saveTimer
        interval: 1000
        repeat: false
        onTriggered: root.checkpointEditor()
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
            width: Math.min(420, root.width - 48)
            wrapMode: Text.WordWrap
            text: qsTr("Delete “%1”?").arg(root.selectedTitle)
        }

        onAccepted: {
            if (!root.workspace.currentEditor || root.checkpointEditor())
                root.workspace.deleteNote(root.selectedStorageId, root.selectedNoteId)
        }
    }

    Dialog {
        id: moveDialog
        parent: root
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        modal: true
        title: qsTr("Move note")
        standardButtons: Dialog.Ok | Dialog.Cancel

        ColumnLayout {
            width: Math.min(420, root.width - 48)
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("Move “%1” to:").arg(root.selectedTitle)
            }
            ComboBox {
                id: destinationStorage
                Layout.fillWidth: true
                model: root.workspace.storages
                textRole: "name"
                valueRole: "storageId"
            }
        }

        onAccepted: {
            if (destinationStorage.currentValue
                    && destinationStorage.currentValue !== root.selectedStorageId
                    && (!root.workspace.currentEditor || root.checkpointEditor())) {
                root.workspace.moveNote(root.selectedStorageId,
                                        root.selectedNoteId,
                                        destinationStorage.currentValue)
            }
        }
    }

    Connections {
        target: root.workspace
        function onCurrentEditorChanged() {
            saveTimer.stop()
            if (root.workspace.currentEditor) {
                root.selectedStorageId = root.workspace.currentStorageId
                root.selectedNoteId = root.workspace.currentNoteId
                root.selectedTitle = root.workspace.currentTitle
            }
            if (root.workspace.currentEditor && root.embeddedEditor) {
                Qt.callLater(function() {
                    if (editorLoader.item)
                        editorLoader.item.blockEditor.focusInitialEditor()
                })
            }
        }
        function onLoadingChanged() {
            if (!root.workspace.loading && root.workspace.currentEditor) {
                root.selectedStorageId = root.workspace.currentStorageId
                root.selectedNoteId = root.workspace.currentNoteId
                root.selectedTitle = root.workspace.currentTitle
            }
        }
    }

    Connections {
        target: root.workspace.notesModel
        function onRowsInserted() {
            Qt.callLater(function() { notesTree.expandRecursively(-1, 1) })
        }
    }
    Connections {
        target: root.Window.window
        function onActiveFocusItemChanged() {
            Qt.callLater(function() {
                const ownsFocus = editorLoader.item
                    && editorLoader.item.blockEditor.documentHistoryOwnsFocus()
                if (root.editorFocusOwned && !ownsFocus)
                    root.checkpointEditor()
                root.editorFocusOwned = ownsFocus
            })
        }
    }

}
