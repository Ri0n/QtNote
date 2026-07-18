pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: root
    required property var blockModel
    clip: true

    ListView {
        id: blocks
        model: root.blockModel
        spacing: 8
        boundsBehavior: Flickable.StopAtBounds
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
            width: blocks.width
            sourceComponent: blockType === 1 ? bulletEditor
                           : blockType === 2 ? checkEditor
                           : blockType === 3 ? tableEditor
                           : blockType === 4 ? imageEditor : textEditor
        }
    }

    component BlockTextArea: TextArea {
        wrapMode: TextEdit.Wrap
        selectByMouse: true
        background: null
        padding: 4
    }

    Component {
        id: textEditor
        BlockTextArea {
            width: blockLoader.width
            text: blockLoader.blockText
            textFormat: TextEdit.MarkdownText
            onTextChanged: if (activeFocus) root.blockModel.setBlockText(blockLoader.index, text)
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
    }

    Component {
        id: bulletEditor
        ColumnLayout {
            width: blockLoader.width
            Repeater {
                model: blockLoader.items
                RowLayout {
                    required property int index
                    required property string modelData
                    Label { text: "•"; Layout.alignment: Qt.AlignTop }
                    BlockTextArea {
                        Layout.fillWidth: true
                        text: modelData
                        textFormat: TextEdit.MarkdownText
                        onTextChanged: if (activeFocus) root.blockModel.setListItem(blockLoader.index, index, text)
                        onLinkActivated: link => Qt.openUrlExternally(link)
                    }
                }
            }
        }
    }

    Component {
        id: checkEditor
        ColumnLayout {
            width: blockLoader.width
            Repeater {
                model: blockLoader.items
                RowLayout {
                    required property int index
                    required property string modelData
                    CheckBox {
                        checked: Boolean(blockLoader.checkedItems[index])
                        onToggled: root.blockModel.setChecked(blockLoader.index, index, checked)
                    }
                    BlockTextArea {
                        Layout.fillWidth: true
                        text: modelData
                        textFormat: TextEdit.MarkdownText
                        onTextChanged: if (activeFocus) root.blockModel.setListItem(blockLoader.index, index, text)
                        onLinkActivated: link => Qt.openUrlExternally(link)
                    }
                }
            }
        }
    }

    Component {
        id: tableEditor
        GridLayout {
            width: blockLoader.width
            columns: blockLoader.table.columns
            Repeater {
                model: blockLoader.table.values
                BlockTextArea {
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true
                    text: modelData
                    onTextChanged: if (activeFocus) root.blockModel.setTableCell(blockLoader.index, index, text)
                }
            }
        }
    }

    Component {
        id: imageEditor
        ColumnLayout {
            width: blockLoader.width
            Image {
                Layout.fillWidth: true
                source: blockLoader.url
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }
            RowLayout {
                BlockTextArea {
                    Layout.fillWidth: true
                    placeholderText: qsTr("Image description")
                    text: blockLoader.alt
                    onTextChanged: if (activeFocus) root.blockModel.setImageAlt(blockLoader.index, text)
                }
                BlockTextArea {
                    Layout.fillWidth: true
                    placeholderText: qsTr("Image URL")
                    text: blockLoader.url
                    onTextChanged: if (activeFocus) root.blockModel.setImageUrl(blockLoader.index, text)
                }
            }
        }
    }
}
