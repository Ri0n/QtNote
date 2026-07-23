import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: root

    required property var editorBackend
    required property var blockEditor
    property var platformBackend: null
    property bool compact: width < 640
    property bool showBackButton: false
    property bool showDeleteButton: false
    signal backRequested()
    signal deleteRequested()

    function runMarkdownCommand(kind, command) {
        if (!root.editorBackend || !root.blockEditor)
            return false
        root.blockEditor.flushPendingEditorChanges()
        const beforeView = root.blockEditor.captureEditorState()
        root.editorBackend.beginHistoryTransaction(kind, beforeView)
        try {
            if (!root.editorBackend.markdown)
                root.editorBackend.markdown = true
            return command()
        } finally {
            root.editorBackend.endHistoryTransaction(root.blockEditor.captureEditorState())
        }
    }

    function insertList(type) {
        const wasMarkdown = root.editorBackend && root.editorBackend.markdown
        const row = root.blockEditor ? root.blockEditor.insertionBlockIndex() : 0
        return runMarkdownCommand("insert-or-convert-list", function() {
            if (wasMarkdown)
                return root.blockEditor.insertListBlock(type)
            root.blockEditor.blockModel.insertList(row, type)
            root.blockEditor.focusBlock(row)
            return true
        })
    }

    function insertTable() {
        const row = root.blockEditor ? root.blockEditor.insertionBlockIndex() : 0
        return runMarkdownCommand("insert-table", function() {
            root.blockEditor.blockModel.insertTable(row)
            root.blockEditor.focusBlock(row)
            return true
        })
    }

    function insertImage() {
        if (!root.platformBackend || !root.editorBackend || !root.blockEditor)
            return false
        root.blockEditor.flushPendingEditorChanges()
        if (!root.editorBackend.markdown)
            root.editorBackend.markdown = true
        // Do not keep a history transaction open across the native file
        // dialog's nested event loop. The platform backend starts the media
        // transaction after the user has selected a file. Recompute the row
        // after a possible format conversion recreated the editor delegates.
        Qt.callLater(function() {
            root.platformBackend.insertImage(root.blockEditor.insertionBlockIndex())
        })
        return true
    }

    implicitHeight: contentRow.implicitHeight + 8

    Flickable {
        anchors.fill: parent
        contentWidth: contentRow.implicitWidth
        contentHeight: height
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        RowLayout {
            id: contentRow
            height: parent.height
            spacing: 2

            ToolButton {
                visible: root.showBackButton
                text: qsTr("‹")
                font.pixelSize: 28
                Accessible.name: qsTr("Back")
                onClicked: root.backRequested()
            }

            ToolButton {
                text: qsTr("↶")
                enabled: root.editorBackend && root.editorBackend.canUndo
                Accessible.name: root.editorBackend && root.editorBackend.undoText.length > 0
                                 ? qsTr("Undo %1").arg(root.editorBackend.undoText) : qsTr("Undo")
                onClicked: root.editorBackend.undo()
            }
            ToolButton {
                text: qsTr("↷")
                enabled: root.editorBackend && root.editorBackend.canRedo
                Accessible.name: root.editorBackend && root.editorBackend.redoText.length > 0
                                 ? qsTr("Redo %1").arg(root.editorBackend.redoText) : qsTr("Redo")
                onClicked: root.editorBackend.redo()
            }

            ToolSeparator { }

            ToolButton {
                text: root.editorBackend && root.editorBackend.markdown ? qsTr("TXT") : qsTr("MD")
                checkable: true
                checked: root.editorBackend ? root.editorBackend.markdown : false
                Accessible.name: checked ? qsTr("Switch to plain text") : qsTr("Switch to Markdown")
                onClicked: {
                    if (!root.editorBackend)
                        return
                    root.blockEditor.flushPendingEditorChanges()
                    root.editorBackend.markdown = !root.editorBackend.markdown
                }
            }

            ToolButton {
                text: qsTr("B")
                font.bold: true
                enabled: root.editorBackend && root.editorBackend.markdown
                Accessible.name: qsTr("Bold")
                onClicked: root.blockEditor.applyActiveInlineStyle("bold")
            }
            ToolButton {
                text: qsTr("I")
                font.italic: true
                enabled: root.editorBackend && root.editorBackend.markdown
                Accessible.name: qsTr("Italic")
                onClicked: root.blockEditor.applyActiveInlineStyle("italic")
            }
            ToolButton {
                text: qsTr("S")
                font.strikeout: true
                enabled: root.editorBackend && root.editorBackend.markdown
                Accessible.name: qsTr("Strikethrough")
                onClicked: root.blockEditor.applyActiveInlineStyle("strike")
            }
            ToolButton {
                text: qsTr("</>")
                enabled: root.editorBackend && root.editorBackend.markdown
                Accessible.name: qsTr("Inline code")
                onClicked: root.blockEditor.applyActiveInlineStyle("code")
            }
            ToolButton {
                text: qsTr("🔗")
                enabled: root.editorBackend && root.editorBackend.markdown
                Accessible.name: qsTr("Link")
                onClicked: root.blockEditor.editActiveLink()
            }

            ToolButton {
                id: headingButton
                text: qsTr("H")
                enabled: root.editorBackend && root.editorBackend.markdown
                Accessible.name: qsTr("Heading")
                onClicked: headingMenu.open()
                Menu {
                    id: headingMenu
                    MenuItem { text: qsTr("Normal text"); onTriggered: root.blockEditor.convertActiveToHeading(0) }
                    MenuItem { text: qsTr("Heading 1"); onTriggered: root.blockEditor.convertActiveToHeading(1) }
                    MenuItem { text: qsTr("Heading 2"); onTriggered: root.blockEditor.convertActiveToHeading(2) }
                    MenuItem { text: qsTr("Heading 3"); onTriggered: root.blockEditor.convertActiveToHeading(3) }
                    MenuItem { text: qsTr("Heading 4"); onTriggered: root.blockEditor.convertActiveToHeading(4) }
                }
            }

            ToolButton {
                id: listButton
                text: qsTr("☷")
                enabled: root.editorBackend !== null
                Accessible.name: qsTr("List")
                onClicked: listMenu.open()
                Menu {
                    id: listMenu
                    MenuItem {
                        text: qsTr("Task list")
                        onTriggered: root.insertList(5)
                    }
                    MenuItem {
                        text: qsTr("Numbered list")
                        onTriggered: root.insertList(1)
                    }
                    MenuItem {
                        text: qsTr("Bullet list")
                        onTriggered: root.insertList(2)
                    }
                }
            }

            ToolButton {
                text: qsTr("▦")
                enabled: root.editorBackend !== null
                Accessible.name: qsTr("Insert table")
                onClicked: root.insertTable()
            }
            ToolButton {
                text: qsTr("🖼")
                visible: root.platformBackend !== null
                enabled: root.platformBackend && root.editorBackend && root.editorBackend.supportsMedia
                Accessible.name: qsTr("Insert image")
                onClicked: root.insertImage()
            }

            ToolSeparator { visible: root.showDeleteButton }

            ToolButton {
                visible: root.showDeleteButton
                text: qsTr("Delete")
                Accessible.name: qsTr("Delete note")
                onClicked: root.deleteRequested()
            }

            Item { Layout.preferredWidth: 4; Layout.fillHeight: true }
        }
    }
}
