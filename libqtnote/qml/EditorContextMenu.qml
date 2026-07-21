import QtQuick
import QtQuick.Controls

Menu {
    id: menu
    required property var controller
    required property var editorBackend
    readonly property var editor: controller.contextEditor

    Instantiator {
        model: menu.editor ? menu.editor.contextSuggestions : []
        delegate: MenuItem {
            required property string modelData
            text: modelData
            onTriggered: if (menu.editor) menu.editor.replaceContextWord(modelData)
        }
        onObjectAdded: function(index, object) { menu.insertItem(index, object) }
        onObjectRemoved: function(index, object) { menu.removeItem(object) }
    }
    MenuSeparator { visible: menu.editor && menu.editor.contextWord.length > 0 }
    MenuItem { visible: menu.editor && menu.editor.tableCell; text: qsTr("Insert row above"); onTriggered: menu.editor.insertRowAbove() }
    MenuItem { visible: menu.editor && menu.editor.tableCell; text: qsTr("Insert row below"); onTriggered: menu.editor.insertRowBelow() }
    MenuItem {
        visible: menu.editor && menu.editor.tableCell
        text: qsTr("Delete row")
        enabled: menu.editor && menu.editor.canRemoveTableRow
        onTriggered: menu.editor.removeRow()
    }
    MenuSeparator { visible: menu.editor && menu.editor.tableCell }
    MenuItem { visible: menu.editor && menu.editor.tableCell; text: qsTr("Insert column left"); onTriggered: menu.editor.insertColumnLeft() }
    MenuItem { visible: menu.editor && menu.editor.tableCell; text: qsTr("Insert column right"); onTriggered: menu.editor.insertColumnRight() }
    MenuItem {
        visible: menu.editor && menu.editor.tableCell
        text: qsTr("Delete column")
        enabled: menu.editor && menu.editor.canRemoveTableColumn
        onTriggered: menu.editor.removeColumn()
    }
    MenuSeparator { visible: menu.editor && menu.editor.tableCell }
    MenuItem {
        visible: menu.editor && menu.editor.contextWord.length > 0
        text: qsTr("Add to dictionary")
        onTriggered: {
            menu.editorBackend.addToSpellingDictionary(menu.editor.contextWord)
            menu.editor.refreshSpelling()
        }
    }
    MenuSeparator { visible: menu.editor && menu.editor.contextWord.length > 0 }
    MenuItem {
        text: qsTr("Spell Check")
        checkable: true
        checked: menu.editorBackend ? menu.editorBackend.spellCheckEnabled : false
        onToggled: if (menu.editorBackend) menu.editorBackend.spellCheckEnabled = checked
    }
    MenuSeparator {}
    MenuItem { action: Action { text: qsTr("Undo"); shortcut: StandardKey.Undo; enabled: menu.editor ? menu.editor.canUndo : false; onTriggered: if (menu.editor) menu.editor.undo() } }
    MenuItem { action: Action { text: qsTr("Redo"); shortcut: StandardKey.Redo; enabled: menu.editor ? menu.editor.canRedo : false; onTriggered: if (menu.editor) menu.editor.redo() } }
    MenuSeparator {}
    MenuItem { action: Action { text: qsTr("Cut"); shortcut: StandardKey.Cut; enabled: menu.controller.documentSelectionAvailable; onTriggered: menu.controller.cutDocumentSelection() } }
    MenuItem { action: Action { text: qsTr("Copy"); shortcut: StandardKey.Copy; enabled: menu.controller.documentSelectionAvailable; onTriggered: menu.controller.copyDocumentSelection() } }
    MenuItem {
        action: Action {
            text: qsTr("Paste")
            shortcut: StandardKey.Paste
            enabled: menu.editor ? menu.editor.canPaste : false
            onTriggered: {
                if (!menu.editor)
                    return
                // Calling TextArea.paste() directly bypasses BlockTextArea's
                // key handler and lets QTextDocument create an opaque rich
                // text table.  Give the block model first chance to consume
                // every structural clipboard format; retain Qt's normal text
                // paste only as the fallback.
                menu.controller.activeEditor = menu.editor
                menu.controller.pasteClipboard()
            }
        }
    }
    MenuSeparator {}
    MenuItem { action: Action { text: qsTr("Select All"); shortcut: StandardKey.SelectAll; onTriggered: menu.controller.selectAllDocument() } }
}
