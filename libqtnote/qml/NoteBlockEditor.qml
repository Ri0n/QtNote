import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
    id: root
    property var blockModel: noteBlockModel
    property var activeEditor: null
    model: blockModel
    spacing: 8
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    activeFocusOnTab: true
    focus: true

    function insertTextAtCursor(value) {
        if (!activeEditor)
            return false
        const position = activeEditor.cursorPosition
        activeEditor.insert(position, value)
        activeEditor.cursorPosition = position + value.length
        return true
    }

    function focusInitialEditor() {
        if (activeEditor) {
            activeEditor.forceActiveFocus()
            return true
        }
        const loader = itemAtIndex(0)
        if (!loader || !loader.item)
            return false
        loader.item.forceActiveFocus()
        return true
    }

    delegate: Loader {
        id: blockLoader
        required property int index
        required property int blockType
        required property string blockText
        required property var items
        required property var checkedItems
        required property var table
        required property string url
        required property string alt
        required property url previewUrl
        width: root.width
        height: Math.max(item ? item.implicitHeight : 0,
                         blockType === 0 && index === root.count - 1 ? root.height - y : 0)
        onLoaded: {
            if (blockType === 0 && index === 0 && blockText.trim().length === 0)
                item.forceActiveFocus()
        }
        sourceComponent: blockType === 1 ? bulletEditor
                       : blockType === 2 ? checkEditor
                       : blockType === 3 ? tableEditor
                       : blockType === 4 ? imageEditor : textEditor
    }

    component BlockTextArea: TextArea {
        wrapMode: TextEdit.Wrap
        verticalAlignment: TextEdit.AlignTop
        selectByMouse: true
        background: null
        padding: 4
        onActiveFocusChanged: if (activeFocus) root.activeEditor = this
    }

    Component {
        id: textEditor
        BlockTextArea {
            property var block: parent
            width: block.width
            text: block.blockText
            textFormat: root.blockModel && root.blockModel.markdown ? TextEdit.MarkdownText : TextEdit.PlainText
            onTextChanged: if (activeFocus) root.blockModel.setBlockText(block.index, text)
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
    }

    Component {
        id: bulletEditor
        ColumnLayout {
            id: bulletRoot
            property var block: parent
            width: block.width
            Repeater {
                model: bulletRoot.block.items
                RowLayout {
                    required property int index
                    required property string modelData
                    Label { text: "•"; Layout.alignment: Qt.AlignTop }
                    BlockTextArea {
                        Layout.fillWidth: true
                        text: modelData
                        textFormat: TextEdit.MarkdownText
                        onTextChanged: if (activeFocus) root.blockModel.setListItem(bulletRoot.block.index, index, text)
                        onLinkActivated: link => Qt.openUrlExternally(link)
                    }
                }
            }
        }
    }

    Component {
        id: checkEditor
        ColumnLayout {
            id: checkRoot
            property var block: parent
            width: block.width
            Repeater {
                model: checkRoot.block.items
                RowLayout {
                    required property int index
                    required property string modelData
                    CheckBox {
                        checked: Boolean(checkRoot.block.checkedItems[index])
                        onToggled: root.blockModel.setChecked(checkRoot.block.index, index, checked)
                    }
                    BlockTextArea {
                        Layout.fillWidth: true
                        text: modelData
                        textFormat: TextEdit.MarkdownText
                        onTextChanged: if (activeFocus) root.blockModel.setListItem(checkRoot.block.index, index, text)
                        onLinkActivated: link => Qt.openUrlExternally(link)
                    }
                }
            }
        }
    }

    Component {
        id: tableEditor
        GridLayout {
            id: tableRoot
            property var block: parent
            width: block.width
            columns: block.table.columns
            Repeater {
                model: tableRoot.block.table.values
                BlockTextArea {
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true
                    text: modelData
                    onTextChanged: if (activeFocus) root.blockModel.setTableCell(tableRoot.block.index, index, text)
                }
            }
        }
    }

    Component {
        id: imageEditor
        ColumnLayout {
            id: imageRoot
            property var block: parent
            width: block.width
            Image {
                Layout.fillWidth: true
                source: imageRoot.block.previewUrl
                fillMode: Image.PreserveAspectFit
            }
            RowLayout {
                BlockTextArea {
                    Layout.fillWidth: true
                    placeholderText: qsTr("Image description")
                    text: imageRoot.block.alt
                    onTextChanged: if (activeFocus) root.blockModel.setImageAlt(imageRoot.block.index, text)
                }
                BlockTextArea {
                    Layout.fillWidth: true
                    placeholderText: qsTr("Image URL")
                    text: imageRoot.block.url
                    onTextChanged: if (activeFocus) root.blockModel.setImageUrl(imageRoot.block.index, text)
                }
            }
        }
    }
}
