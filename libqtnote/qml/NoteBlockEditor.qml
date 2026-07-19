import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ListBlockBehavior.js" as ListBlockBehavior
import "TableBlockBehavior.js" as TableBlockBehavior

ListView {
    id: root
    property var blockModel: noteBlockModel
    property var activeEditor: null
    property int pendingFocusBlock: -1
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
    property var keyboardSelectionAnchorEditor: null
    property int keyboardSelectionAnchorPosition: 0
    readonly property int blockSpacing: Math.max(1, Math.round(editorFontMetrics.height * 3 / 5))
    model: blockModel
    spacing: blockSpacing
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    activeFocusOnTab: true
    focus: true

    FontMetrics {
        id: editorFontMetrics
        font: Application.font
    }

    function registerEditor(editor) {
        if (editors.indexOf(editor) < 0) {
            editors = editors.concat([editor])
            ++editorRegistrations
        }
        if (editor.blockIndex === pendingFocusBlock) {
            editor.forceActiveFocus()
            editor.cursorPosition = 0
            activeEditor = editor
            pendingFocusBlock = -1
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

    Timer {
        id: pendingFocusRetry
        interval: 10
        repeat: true
        onTriggered: if (root.focusPendingBlock()) stop()
    }

    EditorContextMenu {
        id: sharedContextMenu
        controller: root
        editorBackend: qmlNoteEditor
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

    function handleKeyboardSelection(event, editor) {
        const arrow = event.key === Qt.Key_Left || event.key === Qt.Key_Right
                   || event.key === Qt.Key_Up || event.key === Qt.Key_Down
        if (!(event.modifiers & Qt.ShiftModifier) || !arrow) {
            if (!(event.modifiers & Qt.ShiftModifier))
                keyboardSelectionAnchorEditor = null
            return false
        }
        if (!keyboardSelectionAnchorEditor) {
            keyboardSelectionAnchorEditor = editor
            keyboardSelectionAnchorPosition = editor.selectionStart !== editor.selectionEnd
                ? (editor.cursorPosition === editor.selectionStart ? editor.selectionEnd : editor.selectionStart)
                : editor.cursorPosition
        }
        let direction = 0
        if (event.key === Qt.Key_Left && editor.cursorPosition === 0)
            direction = -1
        else if (event.key === Qt.Key_Right && editor.cursorPosition === editor.length)
            direction = 1
        else if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
            const rectangle = editor.positionToRectangle(editor.cursorPosition)
            const probeY = event.key === Qt.Key_Up ? rectangle.y - rectangle.height
                                                   : rectangle.y + rectangle.height + 1
            const probe = editor.positionToRectangle(editor.positionAt(rectangle.x, probeY))
            const boundary = event.key === Qt.Key_Up ? probe.y >= rectangle.y - 0.5
                                                     : probe.y <= rectangle.y + 0.5
            if (boundary)
                direction = event.key === Qt.Key_Up ? -1 : 1
        }
        if (direction === 0)
            return false
        const ordered = orderedEditors()
        const index = ordered.indexOf(editor)
        const targetIndex = index + direction
        if (targetIndex < 0 || targetIndex >= ordered.length)
            return true
        const target = ordered[targetIndex]
        const position = direction < 0 ? target.length : 0
        applyDocumentSelection(keyboardSelectionAnchorEditor, keyboardSelectionAnchorPosition, target, position)
        return true
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

    function deleteStructuredSelection() {
        const selected = orderedEditors().filter(editor => editor.selectionStart !== editor.selectionEnd)
        if (selected.length < 2)
            return false
        const block = selected[0].blockIndex
        if (block < 0 || selected.some(editor => editor.blockIndex !== block))
            return false
        const listItems = selected.filter(editor => editor.listItemIndex >= 0)
        if (listItems.length === selected.length) {
            const indexes = listItems.map(editor => editor.listItemIndex)
            blockModel.removeListItems(block, Math.min(...indexes), Math.max(...indexes))
            clearDocumentSelection()
            focusBlock(block)
            return true
        }
        const tableCells = selected.filter(editor => editor.tableRow >= 0)
        if (tableCells.length === selected.length) {
            const rows = tableCells.map(editor => editor.tableRow)
            if (Math.min(...rows) === Math.max(...rows))
                return false
            blockModel.removeTableRows(block, Math.min(...rows), Math.max(...rows))
            clearDocumentSelection()
            focusBlock(block)
            return true
        }
        return false
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
        pendingFocusBlock = blockIndex
        positionViewAtIndex(blockIndex, ListView.Contain)
        pendingFocusRetry.restart()
        Qt.callLater(function() {
            for (const editor of editors) {
                if (editor && editor.blockIndex === blockIndex) {
                    editor.forceActiveFocus()
                    editor.cursorPosition = 0
                    root.activeEditor = editor
                    root.pendingFocusBlock = -1
                    return
                }
            }
            const loader = root.itemAtIndex(blockIndex)
            const directEditor = loader ? loader.item : null
            if (directEditor && directEditor.cursorPosition !== undefined) {
                directEditor.blockIndex = blockIndex
                directEditor.forceActiveFocus()
                directEditor.cursorPosition = 0
                root.activeEditor = directEditor
                root.pendingFocusBlock = -1
            }
        })
    }

    function firstEditorIn(item) {
        if (!item)
            return null
        if (item.objectName === "noteBlockTextArea")
            return item
        for (const child of item.children || []) {
            const editor = firstEditorIn(child)
            if (editor)
                return editor
        }
        return null
    }

    function focusPendingBlock() {
        if (pendingFocusBlock < 0)
            return true
        const loader = itemAtIndex(pendingFocusBlock)
        const editor = firstEditorIn(loader ? loader.item : null)
        if (!editor)
            return false
        editor.blockIndex = pendingFocusBlock
        editor.forceActiveFocus()
        editor.cursorPosition = 0
        activeEditor = editor
        pendingFocusBlock = -1
        return true
    }

    function focusFollowingBlock(blockIndex) {
        const next = blockIndex + 1
        if (next < count) {
            const ordered = orderedEditors()
            const activeIndex = ordered.indexOf(activeEditor)
            if (activeIndex >= 0 && activeIndex + 1 < ordered.length) {
                const editor = ordered[activeIndex + 1]
                editor.blockIndex = next
                editor.forceActiveFocus()
                editor.cursorPosition = 0
                activeEditor = editor
                return
            }
            focusBlock(next)
            return
        }
        pendingFocusBlock = next
        blockModel.appendTextBlock()
        focusBlock(next)
    }

    function focusPrecedingBlock(blockIndex) {
        if (blockIndex <= 0)
            return
        const ordered = orderedEditors()
        const activeIndex = ordered.indexOf(activeEditor)
        if (activeIndex > 0) {
            const editor = ordered[activeIndex - 1]
            editor.blockIndex = blockIndex - 1
            editor.forceActiveFocus()
            editor.cursorPosition = editor.length
            activeEditor = editor
            return
        }
        focusBlock(blockIndex - 1)
    }

    function handleBlockBoundaryNavigation(event, editor) {
        const modifiers = event.modifiers & (Qt.ShiftModifier | Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier)
        if (modifiers || event.key !== Qt.Key_Up || editor.blockIndex <= 0)
            return false
        const rectangle = editor.positionToRectangle(editor.cursorPosition)
        if (rectangle.y > editor.positionToRectangle(0).y + 0.5)
            return false
        focusPrecedingBlock(editor.blockIndex)
        return true
    }

    function insertionBlockIndex() {
        if (pendingFocusBlock >= 0)
            return pendingFocusBlock + 1
        return activeEditor && activeEditor.blockIndex >= 0 ? activeEditor.blockIndex + 1 : count
    }

    function insertTableBlock() {
        const row = insertionBlockIndex()
        blockModel.insertTable(row)
        focusBlock(row)
    }

    function insertListBlock(type) {
        if (activeEditor && activeEditor.blockIndex >= 0) {
            const activeBlock = activeEditor.blockIndex
            if (blockModel.convertListLevel(activeBlock, activeEditor.listItemIndex, type)) {
                return
            }
        }
        const row = insertionBlockIndex()
        blockModel.insertList(row, type)
        focusBlock(row)
    }

    delegate: Loader {
        id: blockLoader
        required property int index
        required property int blockType
        required property string blockText
        required property var items
        required property var checkedItems
        required property var itemIndents
        required property var itemTypes
        required property var table
        required property string url
        required property string alt
        required property url previewUrl
        width: root.width
        height: Math.max(item ? item.implicitHeight : 0,
                         blockType === 0 && index === root.count - 1 ? root.height - y : 0)
        onLoaded: {
            if (item && item.blockIndex !== undefined)
                item.blockIndex = index
            if (item && item.blockIndex === root.pendingFocusBlock) {
                item.forceActiveFocus()
                item.cursorPosition = 0
                root.activeEditor = item
                root.pendingFocusBlock = -1
            }
            if (blockType === 0 && index === 0 && blockText.trim().length === 0)
                item.forceActiveFocus()
        }
        sourceComponent: blockType === 1 || blockType === 2 || blockType === 5 ? listEditor
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
        property int blockIndex: -1
        property int listItemIndex: -1
        property int tableRow: -1
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
            if ((event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace)
                    && root.deleteStructuredSelection()) {
                event.accepted = true
            } else if (root.handleKeyboardSelection(event, blockArea)) {
                event.accepted = true
            } else if (keyHandler && keyHandler(event)) {
                event.accepted = true
            } else if (root.handleBlockBoundaryNavigation(event, blockArea)) {
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
            blockIndex: block.index
            width: block.width
            text: block.blockText
            commitText: function() { root.blockModel.setBlockText(block.index, text) }
            textFormat: root.blockModel && root.blockModel.markdown ? TextEdit.MarkdownText : TextEdit.PlainText
            onTextChanged: if (activeFocus) root.blockModel.setBlockText(block.index, text)
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
    }

    Component {
        id: listEditor
        ColumnLayout {
            id: checkRoot
            property var block: parent
            property var itemData: block.items
            property var checkedData: block.checkedItems
            property var indentData: block.itemIndents
            property var typeData: block.itemTypes
            property bool syncingItems: false
            width: block.width
            onItemDataChanged: syncItems()
            onCheckedDataChanged: syncItems()
            onIndentDataChanged: syncItems()
            onTypeDataChanged: syncItems()
            Component.onCompleted: syncItems()
            function syncItems() {
                syncingItems = true
                const values = itemData || []
                while (checkModel.count > values.length)
                    checkModel.remove(checkModel.count - 1)
                while (checkModel.count < values.length)
                    checkModel.append({ itemText: "", itemChecked: false, itemIndent: 0, itemType: 2 })
                for (let index = 0; index < values.length; ++index) {
                    if (checkModel.get(index).itemText !== values[index])
                        checkModel.setProperty(index, "itemText", values[index])
                    const checked = Boolean(checkedData[index])
                    if (checkModel.get(index).itemChecked !== checked)
                        checkModel.setProperty(index, "itemChecked", checked)
                    const indent = Number(indentData[index] || 0)
                    if (checkModel.get(index).itemIndent !== indent)
                        checkModel.setProperty(index, "itemIndent", indent)
                    const type = Number(typeData[index] === undefined ? block.blockType : typeData[index])
                    if (checkModel.get(index).itemType !== type)
                        checkModel.setProperty(index, "itemType", type)
                }
                syncingItems = false
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
            function itemCount() { return checkModel.count }
            function itemText(index) { return checkModel.get(index).itemText }
            function itemNumber(index) {
                const level = checkModel.get(index).itemIndent
                let number = 1
                for (let previous = index - 1; previous >= 0; --previous) {
                    const item = checkModel.get(previous)
                    if (item.itemIndent < level)
                        break
                    if (item.itemIndent === level && item.itemType === 5)
                        ++number
                }
                return number
            }
            function selectedItemRange() {
                let first = -1
                let last = -1
                for (let index = 0; index < checkModel.count; ++index) {
                    const row = checkRepeater.itemAt(index)
                    if (row && row.listEditor.selectionStart !== row.listEditor.selectionEnd) {
                        if (first < 0)
                            first = index
                        last = index
                    }
                }
                return { first: first, last: last }
            }
            function handleItemKey(event, cell, itemIndex) {
                return ListBlockBehavior.handleKey(checkRoot, root, event, cell, itemIndex)
            }
            Repeater {
                id: checkRepeater
                model: checkModel
                RowLayout {
                    id: checkRow
                    required property int index
                    required property string itemText
                    required property bool itemChecked
                    required property int itemIndent
                    required property int itemType
                    property alias listEditor: checkCell
                    Layout.leftMargin: itemIndent * 20
                    CheckBox {
                        visible: itemType === 2
                        checked: itemChecked
                        onClicked: root.blockModel.setChecked(checkRoot.block.index, index, checked)
                    }
                    Label {
                        visible: itemType !== 2
                        text: itemType === 5 ? checkRoot.itemNumber(index) + "." : "•"
                        Layout.alignment: Qt.AlignTop
                    }
                    BlockTextArea {
                        id: checkCell
                        blockIndex: checkRoot.block.index
                        listItemIndex: checkRow.index
                        Layout.fillWidth: true
                        text: itemText
                        keyHandler: function(event) { return checkRoot.handleItemKey(event, checkCell, checkRow.index) }
                        commitText: function() { root.blockModel.setListItem(checkRoot.block.index, index, text) }
                        textFormat: TextEdit.MarkdownText
                        onTextChanged: if (activeFocus && !checkRoot.syncingItems)
                                           root.blockModel.setListItem(checkRoot.block.index, index, text)
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
            function cellCount() { return cellModel.count }
            function rowCount() { return cellModel.count / columns }
            function cellLength(index) {
                const cell = cellRepeater.itemAt(index)
                return cell ? cell.length : 0
            }
            function rowEmpty(row) {
                for (let column = 0; column < columns; ++column)
                    if (String(tableData.values[row * columns + column] || "").trim().length > 0)
                        return false
                return true
            }
            function handleCellKey(event, cell) {
                return TableBlockBehavior.handleKey(tableRoot, root, event, cell)
            }
            Repeater {
                id: cellRepeater
                model: cellModel
                BlockTextArea {
                    id: tableCell
                    blockIndex: tableRoot.block.index
                    required property int index
                    required property string cellText
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.bold: index < tableRoot.columns
                    text: cellText
                    tableCell: true
                    tableRow: Math.floor(index / tableRoot.columns)
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
                    blockIndex: imageRoot.block.index
                    Layout.fillWidth: true
                    placeholderText: qsTr("Image description")
                    text: imageRoot.block.alt
                    commitText: function() { root.blockModel.setImageAlt(imageRoot.block.index, text) }
                    onTextChanged: if (activeFocus) root.blockModel.setImageAlt(imageRoot.block.index, text)
                }
                BlockTextArea {
                    blockIndex: imageRoot.block.index
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
