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
    property int editorRegistrations: 0
    property int fullSelectionPasses: 0
    property bool selectionSpansEditors: false
    property bool documentSelectionAvailable: false
    property var contextEditor: null
    property bool mouseSelectionActive: false
    model: blockModel
    spacing: 0
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    activeFocusOnTab: true
    focus: true

    function registerEditor(editor) {
        if (editors.indexOf(editor) < 0) {
            editors = editors.concat([editor])
            ++editorRegistrations
        }
    }

    function unregisterEditor(editor) {
        editors = editors.filter(candidate => candidate !== editor)
        selectionStateRefresh.restart()
    }

    function refreshSelectionState() {
        if (wholeDocumentSelected) {
            documentSelectionAvailable = true
            return
        }
        for (const editor of editors) {
            if (editor && editor.selectionStart !== editor.selectionEnd) {
                documentSelectionAvailable = true
                return
            }
        }
        documentSelectionAvailable = false
    }

    Timer {
        id: selectionStateRefresh
        interval: 0
        onTriggered: root.refreshSelectionState()
    }

    Menu {
        id: sharedContextMenu
        Instantiator {
            model: root.contextEditor ? root.contextEditor.contextSuggestions : []
            delegate: MenuItem {
                required property string modelData
                text: modelData
                onTriggered: if (root.contextEditor) root.contextEditor.replaceContextWord(modelData)
            }
            onObjectAdded: function(index, object) { sharedContextMenu.insertItem(index, object) }
            onObjectRemoved: function(index, object) { sharedContextMenu.removeItem(object) }
        }
        MenuSeparator { visible: root.contextEditor && root.contextEditor.contextWord.length > 0 }
        MenuItem {
            visible: root.contextEditor && root.contextEditor.tableCell
            text: qsTr("Insert row above")
            onTriggered: root.contextEditor.insertRowAbove()
        }
        MenuItem {
            visible: root.contextEditor && root.contextEditor.tableCell
            text: qsTr("Insert row below")
            onTriggered: root.contextEditor.insertRowBelow()
        }
        MenuItem {
            visible: root.contextEditor && root.contextEditor.tableCell
            text: qsTr("Delete row")
            enabled: root.contextEditor && root.contextEditor.canRemoveTableRow
            onTriggered: root.contextEditor.removeRow()
        }
        MenuSeparator { visible: root.contextEditor && root.contextEditor.tableCell }
        MenuItem {
            visible: root.contextEditor && root.contextEditor.tableCell
            text: qsTr("Insert column left")
            onTriggered: root.contextEditor.insertColumnLeft()
        }
        MenuItem {
            visible: root.contextEditor && root.contextEditor.tableCell
            text: qsTr("Insert column right")
            onTriggered: root.contextEditor.insertColumnRight()
        }
        MenuItem {
            visible: root.contextEditor && root.contextEditor.tableCell
            text: qsTr("Delete column")
            enabled: root.contextEditor && root.contextEditor.canRemoveTableColumn
            onTriggered: root.contextEditor.removeColumn()
        }
        MenuSeparator { visible: root.contextEditor && root.contextEditor.tableCell }
        MenuItem {
            visible: root.contextEditor && root.contextEditor.contextWord.length > 0
            text: qsTr("Add to dictionary")
            onTriggered: {
                qmlNoteEditor.addToSpellingDictionary(root.contextEditor.contextWord)
                root.contextEditor.refreshSpelling()
            }
        }
        MenuSeparator { visible: root.contextEditor && root.contextEditor.contextWord.length > 0 }
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
                enabled: root.contextEditor ? root.contextEditor.canUndo : false
                onTriggered: if (root.contextEditor) root.contextEditor.undo()
            }
        }
        MenuItem {
            action: Action {
                text: qsTr("Redo")
                shortcut: StandardKey.Redo
                enabled: root.contextEditor ? root.contextEditor.canRedo : false
                onTriggered: if (root.contextEditor) root.contextEditor.redo()
            }
        }
        MenuSeparator {}
        MenuItem {
            action: Action {
                text: qsTr("Cut")
                shortcut: StandardKey.Cut
                enabled: root.documentSelectionAvailable
                onTriggered: root.cutDocumentSelection()
            }
        }
        MenuItem {
            action: Action {
                text: qsTr("Copy")
                shortcut: StandardKey.Copy
                enabled: root.documentSelectionAvailable
                onTriggered: root.copyDocumentSelection()
            }
        }
        MenuItem {
            action: Action {
                text: qsTr("Paste")
                shortcut: StandardKey.Paste
                enabled: root.contextEditor ? root.contextEditor.canPaste : false
                onTriggered: if (root.contextEditor) root.contextEditor.paste()
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

    function activeEditorIndex() { return orderedEditors().indexOf(activeEditor) }

    function editorIsBold(index) {
        const ordered = orderedEditors()
        return index >= 0 && index < ordered.length ? ordered[index].font.bold : false
    }

    function selectedEditorCount() {
        let count = 0
        for (const editor of editors) {
            if (!editor)
                continue
            if (editor.selectionStart !== editor.selectionEnd)
                ++count
        }
        return count
    }

    function editorAtPoint(x, y) {
        ++fullSelectionPasses
        const ordered = orderedEditors()
        let nearest = null
        let nearestDistance = Number.POSITIVE_INFINITY
        for (const editor of ordered) {
            const local = editor.mapFromItem(root, x, y)
            if (local.x >= 0 && local.y >= 0 && local.x <= editor.width && local.y <= editor.height)
                return { editor: editor, position: editor.positionAt(local.x, local.y) }
            const dx = local.x < 0 ? -local.x : (local.x > editor.width ? local.x - editor.width : 0)
            const dy = local.y < 0 ? -local.y : (local.y > editor.height ? local.y - editor.height : 0)
            const distance = dx * dx + dy * dy
            if (distance < nearestDistance) {
                nearestDistance = distance
                nearest = {
                    editor: editor,
                    x: Math.max(0, Math.min(editor.width, local.x)),
                    y: Math.max(0, Math.min(editor.height, local.y))
                }
            }
        }
        if (!nearest)
            return null
        return { editor: nearest.editor, position: nearest.editor.positionAt(nearest.x, nearest.y) }
    }

    function clearDocumentSelection() {
        wholeDocumentSelected = false
        selectionSpansEditors = false
        documentSelectionAvailable = false
        for (const editor of editors) {
            if (!editor)
                continue
            if (editor.selectionStart !== editor.selectionEnd)
                editor.select(editor.cursorPosition, editor.cursorPosition)
        }
    }

    function setEditorSelection(editor, start, end) {
        const selectionStart = Math.min(start, end)
        const selectionEnd = Math.max(start, end)
        if (editor.selectionStart === selectionStart && editor.selectionEnd === selectionEnd)
            return
        editor.select(start, end)
    }

    function applyDocumentSelection(anchorEditor, anchorPosition, focusEditor, focusPosition) {
        wholeDocumentSelected = false
        if (anchorEditor === focusEditor) {
            if (selectionSpansEditors) {
                for (const editor of editors) {
                    if (editor && editor !== focusEditor && editor.selectionStart !== editor.selectionEnd)
                        editor.select(editor.cursorPosition, editor.cursorPosition)
                }
            }
            selectionSpansEditors = false
            setEditorSelection(focusEditor, anchorPosition, focusPosition)
            documentSelectionAvailable = anchorPosition !== focusPosition
            if (activeEditor !== focusEditor) {
                focusEditor.forceActiveFocus()
                activeEditor = focusEditor
            }
            return
        }
        const ordered = orderedEditors()
        const anchorIndex = ordered.indexOf(anchorEditor)
        const focusIndex = ordered.indexOf(focusEditor)
        if (anchorIndex < 0 || focusIndex < 0)
            return
        selectionSpansEditors = true
        documentSelectionAvailable = true
        const forward = anchorIndex < focusIndex || (anchorIndex === focusIndex && anchorPosition <= focusPosition)
        const firstIndex = forward ? anchorIndex : focusIndex
        const lastIndex = forward ? focusIndex : anchorIndex
        for (let index = 0; index < ordered.length; ++index) {
            const editor = ordered[index]
            if (index < firstIndex || index > lastIndex) {
                if (editor.selectionStart !== editor.selectionEnd)
                    editor.select(editor.cursorPosition, editor.cursorPosition)
            } else if (anchorIndex === focusIndex) {
                setEditorSelection(editor, anchorPosition, focusPosition)
            } else if (index === anchorIndex) {
                setEditorSelection(editor, anchorPosition, forward ? editor.length : 0)
            } else if (index === focusIndex) {
                setEditorSelection(editor, forward ? 0 : editor.length, focusPosition)
            } else {
                setEditorSelection(editor, 0, editor.length)
            }
        }
        if (activeEditor !== focusEditor) {
            focusEditor.forceActiveFocus()
            activeEditor = focusEditor
        }
    }

    function hasDocumentSelection() {
        return documentSelectionAvailable
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
        documentSelectionAvailable = true
        for (const editor of orderedEditors())
            setEditorSelection(editor, 0, editor.length)
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

    function focusBlock(blockIndex) {
        Qt.callLater(function() {
            const loader = root.itemAtIndex(blockIndex)
            if (!loader || !loader.item)
                return
            loader.item.forceActiveFocus()
            root.activeEditor = loader.item
        })
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
        property var keyHandler: null
        property bool tableCell: false
        property bool canRemoveTableRow: false
        property bool canRemoveTableColumn: false
        property var insertRowAbove: null
        property var insertRowBelow: null
        property var removeRow: null
        property var insertColumnLeft: null
        property var insertColumnRight: null
        property var removeColumn: null
        wrapMode: TextEdit.Wrap
        verticalAlignment: TextEdit.AlignTop
        selectByMouse: true
        persistentSelection: true
        background: null
        leftPadding: 4
        rightPadding: 4
        topPadding: 0
        bottomPadding: 0
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
            function onSelectedTextChanged() {
                if (!root.mouseSelectionActive)
                    selectionStateRefresh.restart()
            }
        }

        Keys.onPressed: function(event) {
            if (keyHandler && keyHandler(event)) {
                event.accepted = true
            } else if (event.matches(StandardKey.SelectAll)) {
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

        function refreshSpelling() { spellRefresh.restart() }

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
                    root.clearDocumentSelection()
                    root.setEditorSelection(blockArea, position, position)
                    root.mouseSelectionActive = true
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
                root.contextEditor = blockArea
                sharedContextMenu.popup()
                mouse.accepted = true
            }
            onPositionChanged: function(mouse) {
                if (!selecting || !(mouse.buttons & Qt.LeftButton))
                    return
                const started = Date.now()
                selectionMoved = true
                if (mouse.x >= 0 && mouse.y >= 0 && mouse.x <= blockArea.width && mouse.y <= blockArea.height) {
                    root.applyDocumentSelection(root.selectionAnchorEditor, root.selectionAnchorPosition,
                                                blockArea, blockArea.positionAt(mouse.x, mouse.y))
                    const elapsed = Date.now() - started
                    if (elapsed >= 8)
                        console.info("QML selection move slow: path=local duration=" + elapsed
                                     + "ms editors=" + root.editors.length)
                    return
                }
                const globalPosition = blockArea.mapToItem(root, mouse.x, mouse.y)
                const hit = root.editorAtPoint(globalPosition.x, globalPosition.y)
                if (hit)
                    root.applyDocumentSelection(root.selectionAnchorEditor, root.selectionAnchorPosition,
                                                hit.editor, hit.position)
                const elapsed = Date.now() - started
                if (elapsed >= 8)
                    console.info("QML selection move slow: path=cross-editor duration=" + elapsed
                                 + "ms editors=" + root.editors.length)
            }
            onReleased: function(mouse) {
                if (mouse.button === Qt.LeftButton) {
                    if (!selectionMoved) {
                        const link = blockArea.linkAt(mouse.x, mouse.y)
                        if (link.length > 0)
                            Qt.openUrlExternally(link)
                    }
                    selecting = false
                    root.mouseSelectionActive = false
                    selectionStateRefresh.restart()
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
            property var itemData: block.items
            width: block.width
            onItemDataChanged: syncItems()
            Component.onCompleted: syncItems()
            function syncItems() {
                const values = itemData || []
                while (bulletModel.count > values.length)
                    bulletModel.remove(bulletModel.count - 1)
                while (bulletModel.count < values.length)
                    bulletModel.append({ itemText: "" })
                for (let index = 0; index < values.length; ++index) {
                    if (bulletModel.get(index).itemText !== values[index])
                        bulletModel.setProperty(index, "itemText", values[index])
                }
            }
            ListModel { id: bulletModel }
            function focusItem(itemIndex, position) {
                Qt.callLater(function() {
                    const row = bulletRepeater.itemAt(itemIndex)
                    const cell = row ? row.listEditor : null
                    if (!cell)
                        return
                    cell.forceActiveFocus()
                    cell.cursorPosition = Math.max(0, Math.min(cell.length, position))
                    root.activeEditor = cell
                })
            }
            function focusItemVertically(itemIndex, x, atBottom) {
                Qt.callLater(function() {
                    const row = bulletRepeater.itemAt(itemIndex)
                    const cell = row ? row.listEditor : null
                    if (!cell)
                        return
                    cell.forceActiveFocus()
                    const y = atBottom ? Math.max(0, cell.height - cell.bottomPadding - 1)
                                       : cell.topPadding + 1
                    cell.cursorPosition = cell.positionAt(x, y)
                    root.activeEditor = cell
                })
            }
            function handleItemKey(event, cell, itemIndex) {
                const blockedModifiers = event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier)
                if (!blockedModifiers && !(event.modifiers & Qt.ShiftModifier)
                        && (event.key === Qt.Key_Up || event.key === Qt.Key_Down)) {
                    const cursorRectangle = cell.positionToRectangle(cell.cursorPosition)
                    const probeY = event.key === Qt.Key_Up
                        ? cursorRectangle.y - cursorRectangle.height
                        : cursorRectangle.y + cursorRectangle.height + 1
                    const probePosition = cell.positionAt(cursorRectangle.x, probeY)
                    const probeRectangle = cell.positionToRectangle(probePosition)
                    const atVisualBoundary = event.key === Qt.Key_Up
                        ? probeRectangle.y >= cursorRectangle.y - 0.5
                        : probeRectangle.y <= cursorRectangle.y + 0.5
                    if (event.key === Qt.Key_Up && atVisualBoundary) {
                        if (itemIndex > 0)
                            focusItemVertically(itemIndex - 1, cursorRectangle.x, true)
                        return true
                    }
                    if (event.key === Qt.Key_Down && atVisualBoundary) {
                        if (itemIndex + 1 < bulletModel.count)
                            focusItemVertically(itemIndex + 1, cursorRectangle.x, false)
                        return true
                    }
                }
                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                        && !blockedModifiers && !(event.modifiers & Qt.ShiftModifier)) {
                    const position = cell.cursorPosition
                    const right = cell.text.substring(position)
                    root.blockModel.setListItem(block.index, itemIndex, cell.text.substring(0, position))
                    root.blockModel.insertListItem(block.index, itemIndex + 1, right)
                    focusItem(itemIndex + 1, 0)
                    return true
                }
                if (event.key === Qt.Key_Backspace && !blockedModifiers && cell.length === 0
                        && cell.cursorPosition === 0 && bulletModel.count === 1) {
                    root.blockModel.convertListToText(block.index)
                    root.focusBlock(block.index)
                    return true
                }
                if (event.key === Qt.Key_Backspace && !blockedModifiers && cell.length === 0
                        && cell.cursorPosition === 0 && bulletModel.count > 1) {
                    const target = itemIndex > 0 ? itemIndex - 1 : 0
                    const targetLength = itemIndex > 0 ? bulletModel.get(target).itemText.length : 0
                    root.blockModel.removeListItem(block.index, itemIndex)
                    focusItem(target, targetLength)
                    return true
                }
                return false
            }
            Repeater {
                id: bulletRepeater
                model: bulletModel
                RowLayout {
                    id: bulletRow
                    required property int index
                    required property string itemText
                    property alias listEditor: bulletCell
                    Label { text: "•"; Layout.alignment: Qt.AlignTop }
                    BlockTextArea {
                        id: bulletCell
                        Layout.fillWidth: true
                        text: itemText
                        keyHandler: function(event) { return bulletRoot.handleItemKey(event, bulletCell, bulletRow.index) }
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
            property var itemData: block.items
            property var checkedData: block.checkedItems
            width: block.width
            onItemDataChanged: syncItems()
            onCheckedDataChanged: syncItems()
            Component.onCompleted: syncItems()
            function syncItems() {
                const values = itemData || []
                while (checkModel.count > values.length)
                    checkModel.remove(checkModel.count - 1)
                while (checkModel.count < values.length)
                    checkModel.append({ itemText: "", itemChecked: false })
                for (let index = 0; index < values.length; ++index) {
                    if (checkModel.get(index).itemText !== values[index])
                        checkModel.setProperty(index, "itemText", values[index])
                    const checked = Boolean(checkedData[index])
                    if (checkModel.get(index).itemChecked !== checked)
                        checkModel.setProperty(index, "itemChecked", checked)
                }
            }
            ListModel { id: checkModel }
            function focusItem(itemIndex, position) {
                Qt.callLater(function() {
                    const row = checkRepeater.itemAt(itemIndex)
                    const cell = row ? row.listEditor : null
                    if (!cell)
                        return
                    cell.forceActiveFocus()
                    cell.cursorPosition = Math.max(0, Math.min(cell.length, position))
                    root.activeEditor = cell
                })
            }
            function focusItemVertically(itemIndex, x, atBottom) {
                Qt.callLater(function() {
                    const row = checkRepeater.itemAt(itemIndex)
                    const cell = row ? row.listEditor : null
                    if (!cell)
                        return
                    cell.forceActiveFocus()
                    const y = atBottom ? Math.max(0, cell.height - cell.bottomPadding - 1)
                                       : cell.topPadding + 1
                    cell.cursorPosition = cell.positionAt(x, y)
                    root.activeEditor = cell
                })
            }
            function handleItemKey(event, cell, itemIndex) {
                const blockedModifiers = event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier)
                if (!blockedModifiers && !(event.modifiers & Qt.ShiftModifier)
                        && (event.key === Qt.Key_Up || event.key === Qt.Key_Down)) {
                    const cursorRectangle = cell.positionToRectangle(cell.cursorPosition)
                    const probeY = event.key === Qt.Key_Up
                        ? cursorRectangle.y - cursorRectangle.height
                        : cursorRectangle.y + cursorRectangle.height + 1
                    const probePosition = cell.positionAt(cursorRectangle.x, probeY)
                    const probeRectangle = cell.positionToRectangle(probePosition)
                    const atVisualBoundary = event.key === Qt.Key_Up
                        ? probeRectangle.y >= cursorRectangle.y - 0.5
                        : probeRectangle.y <= cursorRectangle.y + 0.5
                    if (event.key === Qt.Key_Up && atVisualBoundary) {
                        if (itemIndex > 0)
                            focusItemVertically(itemIndex - 1, cursorRectangle.x, true)
                        return true
                    }
                    if (event.key === Qt.Key_Down && atVisualBoundary) {
                        if (itemIndex + 1 < checkModel.count)
                            focusItemVertically(itemIndex + 1, cursorRectangle.x, false)
                        return true
                    }
                }
                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                        && !blockedModifiers && !(event.modifiers & Qt.ShiftModifier)) {
                    const position = cell.cursorPosition
                    const right = cell.text.substring(position)
                    root.blockModel.setListItem(block.index, itemIndex, cell.text.substring(0, position))
                    root.blockModel.insertListItem(block.index, itemIndex + 1, right)
                    focusItem(itemIndex + 1, 0)
                    return true
                }
                if (event.key === Qt.Key_Backspace && !blockedModifiers && cell.length === 0
                        && cell.cursorPosition === 0 && checkModel.count === 1) {
                    root.blockModel.convertListToText(block.index)
                    root.focusBlock(block.index)
                    return true
                }
                if (event.key === Qt.Key_Backspace && !blockedModifiers && cell.length === 0
                        && cell.cursorPosition === 0 && checkModel.count > 1) {
                    const target = itemIndex > 0 ? itemIndex - 1 : 0
                    const targetLength = itemIndex > 0 ? checkModel.get(target).itemText.length : 0
                    root.blockModel.removeListItem(block.index, itemIndex)
                    focusItem(target, targetLength)
                    return true
                }
                return false
            }
            Repeater {
                id: checkRepeater
                model: checkModel
                RowLayout {
                    id: checkRow
                    required property int index
                    required property string itemText
                    required property bool itemChecked
                    property alias listEditor: checkCell
                    CheckBox {
                        checked: itemChecked
                        onClicked: root.blockModel.setChecked(checkRoot.block.index, index, checked)
                    }
                    BlockTextArea {
                        id: checkCell
                        Layout.fillWidth: true
                        text: itemText
                        keyHandler: function(event) { return checkRoot.handleItemKey(event, checkCell, checkRow.index) }
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
            property var tableData: block.table
            width: block.width
            columns: tableData.columns
            columnSpacing: 0
            rowSpacing: 0
            onTableDataChanged: syncCells()
            Component.onCompleted: syncCells()
            function syncCells() {
                const values = tableData.values || []
                while (cellModel.count > values.length)
                    cellModel.remove(cellModel.count - 1)
                while (cellModel.count < values.length)
                    cellModel.append({ cellText: "" })
                for (let index = 0; index < values.length; ++index) {
                    const value = values[index] || ""
                    if (cellModel.get(index).cellText !== value)
                        cellModel.setProperty(index, "cellText", value)
                }
            }
            ListModel { id: cellModel }
            function focusCell(cellIndex, position) {
                Qt.callLater(function() {
                    const cell = cellRepeater.itemAt(cellIndex)
                    if (!cell)
                        return
                    cell.forceActiveFocus()
                    cell.cursorPosition = Math.max(0, Math.min(cell.length, position || 0))
                    root.activeEditor = cell
                })
            }
            function focusCellVertically(cellIndex, x, atBottom) {
                Qt.callLater(function() {
                    const cell = cellRepeater.itemAt(cellIndex)
                    if (!cell)
                        return
                    cell.forceActiveFocus()
                    const y = atBottom ? Math.max(0, cell.height - cell.bottomPadding - 1)
                                       : cell.topPadding + 1
                    cell.cursorPosition = cell.positionAt(x, y)
                    root.activeEditor = cell
                })
            }
            function insertRow(tableRow, focusColumn) {
                root.blockModel.insertTableRow(block.index, tableRow)
                focusCell(tableRow * columns + focusColumn, 0)
            }
            function handleCellKey(event, cell) {
                const tableRow = Math.floor(cell.index / columns)
                const column = cell.index % columns
                const navigationModifiers = event.modifiers
                    & (Qt.ShiftModifier | Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier)
                if (!navigationModifiers && event.key === Qt.Key_Left && cell.cursorPosition === 0) {
                    if (cell.index > 0) {
                        const previous = cellRepeater.itemAt(cell.index - 1)
                        focusCell(cell.index - 1, previous ? previous.length : 0)
                    }
                    return true
                }
                if (!navigationModifiers && event.key === Qt.Key_Right && cell.cursorPosition === cell.length) {
                    if (cell.index + 1 < cellModel.count)
                        focusCell(cell.index + 1, 0)
                    return true
                }
                if (!navigationModifiers && (event.key === Qt.Key_Up || event.key === Qt.Key_Down)) {
                    const cursorRectangle = cell.positionToRectangle(cell.cursorPosition)
                    const firstRectangle = cell.positionToRectangle(0)
                    const lastRectangle = cell.positionToRectangle(cell.length)
                    if (event.key === Qt.Key_Up && cursorRectangle.y <= firstRectangle.y + 0.5) {
                        if (tableRow > 0)
                            focusCellVertically(cell.index - columns, cursorRectangle.x, true)
                        return true
                    }
                    if (event.key === Qt.Key_Down && cursorRectangle.y >= lastRectangle.y - 0.5) {
                        if (cell.index + columns < cellModel.count)
                            focusCellVertically(cell.index + columns, cursorRectangle.x, false)
                        return true
                    }
                }
                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                        && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))) {
                    if (event.modifiers & Qt.ShiftModifier)
                        return false
                    if (cell.cursorPosition === cell.length) {
                        insertRow(tableRow + 1, column)
                        return true
                    }
                }
                if (event.key === Qt.Key_Tab
                        && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        if (cell.index > 0)
                            focusCell(cell.index - 1, 0)
                    } else if (cell.index + 1 < cellModel.count) {
                        focusCell(cell.index + 1, 0)
                    } else {
                        insertRow(tableRow + 1, 0)
                    }
                    return true
                }
                return false
            }
            Repeater {
                id: cellRepeater
                model: cellModel
                BlockTextArea {
                    id: tableCell
                    required property int index
                    required property string cellText
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.bold: index < tableRoot.columns
                    text: cellText
                    tableCell: true
                    canRemoveTableRow: cellModel.count / tableRoot.columns > 1
                    canRemoveTableColumn: tableRoot.columns > 1
                    keyHandler: function(event) { return tableRoot.handleCellKey(event, tableCell) }
                    insertRowAbove: function() {
                        tableRoot.insertRow(Math.floor(index / tableRoot.columns), index % tableRoot.columns)
                    }
                    insertRowBelow: function() {
                        tableRoot.insertRow(Math.floor(index / tableRoot.columns) + 1, index % tableRoot.columns)
                    }
                    removeRow: function() {
                        const tableRow = Math.floor(index / tableRoot.columns)
                        const targetRow = Math.min(tableRow, cellModel.count / tableRoot.columns - 2)
                        const target = targetRow * tableRoot.columns + index % tableRoot.columns
                        root.blockModel.removeTableRow(tableRoot.block.index, tableRow)
                        tableRoot.focusCell(target, 0)
                    }
                    insertColumnLeft: function() {
                        const oldColumns = tableRoot.columns
                        const tableRow = Math.floor(index / oldColumns)
                        const column = index % oldColumns
                        root.blockModel.insertTableColumn(tableRoot.block.index, column)
                        tableRoot.focusCell(tableRow * (oldColumns + 1) + column, 0)
                    }
                    insertColumnRight: function() {
                        const oldColumns = tableRoot.columns
                        const tableRow = Math.floor(index / oldColumns)
                        const column = index % oldColumns
                        root.blockModel.insertTableColumn(tableRoot.block.index, column + 1)
                        tableRoot.focusCell(tableRow * (oldColumns + 1) + column + 1, 0)
                    }
                    removeColumn: function() {
                        const oldColumns = tableRoot.columns
                        const tableRow = Math.floor(index / oldColumns)
                        const column = index % oldColumns
                        root.blockModel.removeTableColumn(tableRoot.block.index, column)
                        tableRoot.focusCell(tableRow * (oldColumns - 1) + Math.min(column, oldColumns - 2), 0)
                    }
                    leftPadding: 6
                    rightPadding: 6
                    topPadding: 3
                    bottomPadding: 3
                    background: Rectangle {
                        color: "transparent"
                        border.width: 1
                        border.color: tableCell.palette.mid
                    }
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
