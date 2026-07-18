import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
    id: root
    property var blockModel: noteBlockModel
    property var activeEditor: null
    property var editors: []
    property var selectionAnchorEditor: null
    property int selectionAnchorPosition: 0
    property bool wholeDocumentSelected: false
    model: blockModel
    spacing: 8
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    activeFocusOnTab: true
    focus: true

    function registerEditor(editor) {
        if (editors.indexOf(editor) < 0)
            editors = editors.concat([editor])
    }

    function unregisterEditor(editor) {
        editors = editors.filter(candidate => candidate !== editor)
    }

    function orderedEditors() {
        return editors.filter(editor => editor !== null && editor.visible).sort((left, right) => {
            const lp = left.mapToItem(root, 0, 0)
            const rp = right.mapToItem(root, 0, 0)
            if (Math.abs(lp.y - rp.y) > 1)
                return lp.y - rp.y
            return lp.x - rp.x
        })
    }

    function editorGeometry(index) {
        const ordered = orderedEditors()
        if (index < 0 || index >= ordered.length)
            return {}
        const editor = ordered[index]
        const position = editor.mapToItem(root, 0, 0)
        return { x: position.x, y: position.y, width: editor.width, height: editor.height }
    }

    function selectedEditorCount() {
        let count = 0
        for (const editor of orderedEditors()) {
            if (editor.selectionStart !== editor.selectionEnd)
                ++count
        }
        return count
    }

    function editorAtPoint(x, y) {
        const ordered = orderedEditors()
        for (const editor of ordered) {
            const local = editor.mapFromItem(root, x, y)
            if (local.x >= 0 && local.y >= 0 && local.x <= editor.width && local.y <= editor.height)
                return { editor: editor, position: editor.positionAt(local.x, local.y) }
        }
        if (ordered.length === 0)
            return null
        const firstPos = ordered[0].mapToItem(root, 0, 0)
        if (y < firstPos.y)
            return { editor: ordered[0], position: 0 }
        const last = ordered[ordered.length - 1]
        return { editor: last, position: last.length }
    }

    function clearDocumentSelection() {
        wholeDocumentSelected = false
        for (const editor of orderedEditors())
            editor.select(editor.cursorPosition, editor.cursorPosition)
    }

    function applyDocumentSelection(anchorEditor, anchorPosition, focusEditor, focusPosition) {
        const ordered = orderedEditors()
        const anchorIndex = ordered.indexOf(anchorEditor)
        const focusIndex = ordered.indexOf(focusEditor)
        if (anchorIndex < 0 || focusIndex < 0)
            return
        wholeDocumentSelected = false
        const forward = anchorIndex < focusIndex || (anchorIndex === focusIndex && anchorPosition <= focusPosition)
        const firstIndex = forward ? anchorIndex : focusIndex
        const lastIndex = forward ? focusIndex : anchorIndex
        for (let index = 0; index < ordered.length; ++index) {
            const editor = ordered[index]
            if (index < firstIndex || index > lastIndex) {
                editor.select(editor.cursorPosition, editor.cursorPosition)
            } else if (anchorIndex === focusIndex) {
                editor.select(anchorPosition, focusPosition)
            } else if (index === anchorIndex) {
                editor.select(anchorPosition, forward ? editor.length : 0)
            } else if (index === focusIndex) {
                editor.select(forward ? 0 : editor.length, focusPosition)
            } else {
                editor.select(0, editor.length)
            }
        }
        focusEditor.forceActiveFocus()
        activeEditor = focusEditor
    }

    function hasDocumentSelection() {
        if (wholeDocumentSelected)
            return true
        for (const editor of orderedEditors()) {
            if (editor.selectionStart !== editor.selectionEnd)
                return true
        }
        return false
    }

    function selectedDocumentText() {
        if (wholeDocumentSelected)
            return blockModel ? blockModel.contents : ""
        const parts = []
        for (const editor of orderedEditors()) {
            if (editor.selectionStart !== editor.selectionEnd)
                parts.push(editor.selectedText)
        }
        return parts.join("\n")
    }

    function copyDocumentSelection() {
        const value = selectedDocumentText()
        if (value.length > 0)
            qmlNoteEditor.copyToClipboard(value)
    }

    function cutDocumentSelection() {
        if (!hasDocumentSelection())
            return
        copyDocumentSelection()
        if (wholeDocumentSelected) {
            wholeDocumentSelected = false
            blockModel.contents = ""
            return
        }
        for (const editor of orderedEditors()) {
            if (editor.selectionStart === editor.selectionEnd)
                continue
            const start = editor.selectionStart
            const end = editor.selectionEnd
            editor.remove(start, end)
            editor.cursorPosition = start
            editor.commitText()
        }
    }

    function selectAllDocument() {
        wholeDocumentSelected = true
        for (const editor of orderedEditors())
            editor.select(0, editor.length)
    }

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
        id: blockArea
        objectName: "noteBlockTextArea"
        property bool titleDocument: false
        property var spellingRanges: []
        property string contextWord: ""
        property int contextStart: 0
        property int contextEnd: 0
        property var contextSuggestions: []
        property var commitText: function() {}
        wrapMode: TextEdit.Wrap
        verticalAlignment: TextEdit.AlignTop
        selectByMouse: true
        persistentSelection: true
        background: null
        padding: 4
        onActiveFocusChanged: if (activeFocus) root.activeEditor = this
        Component.onCompleted: {
            root.registerEditor(blockArea)
            qmlNoteEditor.registerTextDocument(textDocument, titleDocument)
            spellRefresh.restart()
        }
        Component.onDestruction: root.unregisterEditor(blockArea)

        Timer {
            id: spellRefresh
            interval: 0
            onTriggered: {
                blockArea.spellingRanges = qmlNoteEditor.spellCheckRanges(blockArea.textDocument)
                spellingCanvas.requestPaint()
            }
        }

        Connections {
            target: blockArea
            function onTextChanged() { spellRefresh.restart() }
        }

        Keys.onPressed: function(event) {
            if (event.matches(StandardKey.SelectAll)) {
                root.selectAllDocument()
                event.accepted = true
            } else if (event.matches(StandardKey.Copy) && root.hasDocumentSelection()) {
                root.copyDocumentSelection()
                event.accepted = true
            } else if (event.matches(StandardKey.Cut) && root.hasDocumentSelection()) {
                root.cutDocumentSelection()
                event.accepted = true
            }
        }

        function isSpellingError(position) {
            for (const range of spellingRanges) {
                if (position >= range.start && position < range.start + range.length)
                    return true
            }
            return false
        }

        function replaceContextWord(replacement) {
            remove(contextStart, contextEnd)
            insert(contextStart, replacement)
            cursorPosition = contextStart + replacement.length
        }

        MouseArea {
            id: editorMouseArea
            anchors.fill: parent
            z: 20
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            preventStealing: true
            property bool selecting: false
            property bool selectionMoved: false
            onPressed: function(mouse) {
                const position = blockArea.positionAt(mouse.x, mouse.y)
                if (mouse.button === Qt.LeftButton) {
                    blockArea.forceActiveFocus()
                    root.activeEditor = blockArea
                    root.selectionAnchorEditor = blockArea
                    root.selectionAnchorPosition = position
                    root.applyDocumentSelection(blockArea, position, blockArea, position)
                    selecting = true
                    selectionMoved = false
                    mouse.accepted = true
                    return
                }
                blockArea.forceActiveFocus()
                root.activeEditor = blockArea
                blockArea.contextWord = ""
                blockArea.contextSuggestions = []
                if (blockArea.isSpellingError(position)) {
                    blockArea.cursorPosition = position
                    blockArea.selectWord()
                    blockArea.contextWord = blockArea.selectedText
                    blockArea.contextStart = blockArea.selectionStart
                    blockArea.contextEnd = blockArea.selectionEnd
                    blockArea.contextSuggestions = qmlNoteEditor.spellingSuggestions(blockArea.contextWord)
                } else if (blockArea.selectionStart === blockArea.selectionEnd
                           || position < blockArea.selectionStart || position > blockArea.selectionEnd) {
                    root.clearDocumentSelection()
                    blockArea.cursorPosition = position
                }
                contextMenu.popup()
                mouse.accepted = true
            }
            onPositionChanged: function(mouse) {
                if (!selecting || !(mouse.buttons & Qt.LeftButton))
                    return
                selectionMoved = true
                const globalPosition = blockArea.mapToItem(root, mouse.x, mouse.y)
                const hit = root.editorAtPoint(globalPosition.x, globalPosition.y)
                if (hit)
                    root.applyDocumentSelection(root.selectionAnchorEditor, root.selectionAnchorPosition,
                                                hit.editor, hit.position)
            }
            onReleased: function(mouse) {
                if (mouse.button === Qt.LeftButton) {
                    if (!selectionMoved) {
                        const link = blockArea.linkAt(mouse.x, mouse.y)
                        if (link.length > 0)
                            Qt.openUrlExternally(link)
                    }
                    selecting = false
                    root.selectionAnchorEditor = null
                }
            }
            onDoubleClicked: function(mouse) {
                if (mouse.button !== Qt.LeftButton)
                    return
                blockArea.cursorPosition = blockArea.positionAt(mouse.x, mouse.y)
                blockArea.selectWord()
                root.wholeDocumentSelected = false
            }
        }

        Menu {
            id: contextMenu
            Instantiator {
                model: blockArea.contextSuggestions
                delegate: MenuItem {
                    required property string modelData
                    text: modelData
                    onTriggered: blockArea.replaceContextWord(modelData)
                }
                onObjectAdded: function(index, object) { contextMenu.insertItem(index, object) }
                onObjectRemoved: function(index, object) { contextMenu.removeItem(object) }
            }
            MenuSeparator { visible: blockArea.contextWord.length > 0 }
            MenuItem {
                visible: blockArea.contextWord.length > 0
                text: qsTr("Add to dictionary")
                onTriggered: {
                    qmlNoteEditor.addToSpellingDictionary(blockArea.contextWord)
                    spellRefresh.restart()
                }
            }
            MenuSeparator { visible: blockArea.contextWord.length > 0 }
            MenuItem {
                text: qsTr("Spell Check")
                checkable: true
                checked: qmlNoteEditor ? qmlNoteEditor.spellCheckEnabled : false
                onToggled: if (qmlNoteEditor) qmlNoteEditor.spellCheckEnabled = checked
            }
            MenuSeparator {}
            MenuItem {
                action: Action {
                    text: qsTr("Undo")
                    shortcut: StandardKey.Undo
                    enabled: blockArea.canUndo
                    onTriggered: blockArea.undo()
                }
            }
            MenuItem {
                action: Action {
                    text: qsTr("Redo")
                    shortcut: StandardKey.Redo
                    enabled: blockArea.canRedo
                    onTriggered: blockArea.redo()
                }
            }
            MenuSeparator {}
            MenuItem {
                action: Action {
                    text: qsTr("Cut")
                    shortcut: StandardKey.Cut
                    enabled: root.hasDocumentSelection()
                    onTriggered: root.cutDocumentSelection()
                }
            }
            MenuItem {
                action: Action {
                    text: qsTr("Copy")
                    shortcut: StandardKey.Copy
                    enabled: root.hasDocumentSelection()
                    onTriggered: root.copyDocumentSelection()
                }
            }
            MenuItem {
                action: Action {
                    text: qsTr("Paste")
                    shortcut: StandardKey.Paste
                    enabled: blockArea.canPaste
                    onTriggered: blockArea.paste()
                }
            }
            MenuSeparator {}
            MenuItem {
                action: Action {
                    text: qsTr("Select All")
                    shortcut: StandardKey.SelectAll
                    onTriggered: root.selectAllDocument()
                }
            }
        }

        Canvas {
            id: spellingCanvas
            anchors.fill: parent
            z: 10
            function drawWave(ctx, left, right, y) {
                if (right <= left)
                    return
                ctx.beginPath()
                ctx.moveTo(left, y)
                let up = true
                for (let x = left + 2; x < right; x += 2) {
                    ctx.lineTo(x, y + (up ? -1.5 : 1.5))
                    up = !up
                }
                ctx.lineTo(right, y)
                ctx.stroke()
            }
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = "#d32f2f"
                ctx.lineWidth = 1.25
                for (const range of blockArea.spellingRanges) {
                    const end = range.start + range.length
                    let segmentLeft = -1
                    let segmentY = 0
                    for (let position = range.start; position < end; ++position) {
                        const current = blockArea.positionToRectangle(position)
                        const next = blockArea.positionToRectangle(position + 1)
                        let right = next.y === current.y ? next.x : current.x + current.width
                        if (right <= current.x)
                            right = current.x + Math.max(2, current.width)
                        const y = current.y + current.height - 1.5
                        if (segmentLeft < 0) {
                            segmentLeft = current.x
                            segmentY = y
                        }
                        if (next.y !== current.y || position === end - 1) {
                            drawWave(ctx, segmentLeft, right, segmentY)
                            segmentLeft = -1
                        }
                    }
                }
            }
        }
    }

    Component {
        id: textEditor
        BlockTextArea {
            property var block: parent
            titleDocument: block.index === 0
            width: block.width
            text: block.blockText
            commitText: function() { root.blockModel.setBlockText(block.index, text) }
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
                        commitText: function() { root.blockModel.setListItem(bulletRoot.block.index, index, text) }
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
                        commitText: function() { root.blockModel.setListItem(checkRoot.block.index, index, text) }
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
                    commitText: function() { root.blockModel.setTableCell(tableRoot.block.index, index, text) }
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
                    commitText: function() { root.blockModel.setImageAlt(imageRoot.block.index, text) }
                    onTextChanged: if (activeFocus) root.blockModel.setImageAlt(imageRoot.block.index, text)
                }
                BlockTextArea {
                    Layout.fillWidth: true
                    placeholderText: qsTr("Image URL")
                    text: imageRoot.block.url
                    commitText: function() { root.blockModel.setImageUrl(imageRoot.block.index, text) }
                    onTextChanged: if (activeFocus) root.blockModel.setImageUrl(imageRoot.block.index, text)
                }
            }
        }
    }
}
