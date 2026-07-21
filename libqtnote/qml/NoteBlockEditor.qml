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
    readonly property int editorInset: Math.max(8, Math.round(editorFontMetrics.height * 2 / 3))
    readonly property int scrollBarInset: verticalScrollBar.visible
                                           ? Math.ceil(verticalScrollBar.width) : 0
    model: blockModel
    spacing: blockSpacing
    clip: true
    topMargin: editorInset
    bottomMargin: editorInset
    boundsBehavior: Flickable.StopAtBounds
    activeFocusOnTab: true
    focus: true

    ScrollBar.vertical: ScrollBar {
        id: verticalScrollBar
        policy: ScrollBar.AsNeeded
    }

    FontMetrics {
        id: editorFontMetrics
        font: Application.font
    }

    function formattedLinkLabelRuns(label) {
        const runs = []
        let buffer = ""
        let bold = false
        let italic = false
        let strike = false
        let codeDelimiter = ""
        let foundFormatting = false

        function appendRun() {
            if (buffer.length === 0)
                return
            runs.push({
                text: buffer,
                bold: bold,
                italic: italic,
                strike: strike,
                code: codeDelimiter.length > 0
            })
            buffer = ""
        }

        function backtickRunAt(position) {
            let end = position
            while (end < label.length && label.charAt(end) === "`")
                ++end
            return label.substring(position, end)
        }

        function isWordCharacter(character) {
            if (!character)
                return false
            return /[0-9A-Za-z]/.test(character)
                    || character.toUpperCase() !== character.toLowerCase()
        }

        function underscoreIsIntraword(position, length) {
            const previous = position > 0 ? label.charAt(position - 1) : ""
            const next = position + length < label.length
                       ? label.charAt(position + length) : ""
            return isWordCharacter(previous) && isWordCharacter(next)
        }

        for (let index = 0; index < label.length;) {
            const character = label.charAt(index)
            if (character === "\\" && index + 1 < label.length) {
                buffer += label.substring(index, index + 2)
                index += 2
                continue
            }

            if (character === "`") {
                const delimiter = backtickRunAt(index)
                if (codeDelimiter.length === 0 || codeDelimiter === delimiter) {
                    appendRun()
                    codeDelimiter = codeDelimiter.length === 0 ? delimiter : ""
                    foundFormatting = true
                    index += delimiter.length
                    continue
                }
            }

            if (codeDelimiter.length === 0) {
                const pair = label.substring(index, index + 2)
                if (pair === "**" || (pair === "__" && !underscoreIsIntraword(index, 2))) {
                    appendRun()
                    bold = !bold
                    foundFormatting = true
                    index += 2
                    continue
                }
                if (pair === "~~") {
                    appendRun()
                    strike = !strike
                    foundFormatting = true
                    index += 2
                    continue
                }
                if (character === "*"
                        || (character === "_" && !underscoreIsIntraword(index, 1))) {
                    appendRun()
                    italic = !italic
                    foundFormatting = true
                    ++index
                    continue
                }
            }

            buffer += character
            ++index
        }

        appendRun()
        if (!foundFormatting || bold || italic || strike || codeDelimiter.length > 0)
            return null
        return runs
    }

    function codeSpanForRendering(text) {
        let maximumRun = 0
        let currentRun = 0
        for (let index = 0; index < text.length; ++index) {
            if (text.charAt(index) === "`") {
                ++currentRun
                maximumRun = Math.max(maximumRun, currentRun)
            } else {
                currentRun = 0
            }
        }
        let delimiter = ""
        for (let index = 0; index <= maximumRun; ++index)
            delimiter += "`"
        return delimiter + text + delimiter
    }

    function renderFormattedLinkRun(run) {
        let value = run.code ? codeSpanForRendering(run.text) : run.text
        if (run.italic)
            value = "*" + value + "*"
        if (run.bold)
            value = "**" + value + "**"
        if (run.strike)
            value = "~~" + value + "~~"
        return value
    }

    function markdownForRendering(source) {
        if (!source || (source.indexOf("*") < 0 && source.indexOf("_") < 0
                        && source.indexOf("~") < 0 && source.indexOf("`") < 0))
            return source

        // QTextDocument can lose nested or intraword formatting inside a
        // link label. Give every uniform style run its own adjacent link.
        // The C++ serializer joins the runs back into one Markdown link.
        const linkPattern = /\[((?:\\.|[^\]\\\n])*)\]\(([^)\n]+)\)/g
        return source.replace(linkPattern, function(fullMatch, label, destination, offset, wholeText) {
            if (offset > 0 && wholeText.charAt(offset - 1) === "!")
                return fullMatch

            const runs = formattedLinkLabelRuns(label)
            if (runs === null)
                return fullMatch

            let result = ""
            for (const run of runs) {
                if (run.text.length === 0)
                    continue
                result += "[" + renderFormattedLinkRun(run) + "](" + destination + ")"
            }
            return result.length > 0 ? result : fullMatch
        })
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

    function selectedDocumentMarkdown() {
        if (wholeDocumentSelected)
            return blockModel ? blockModel.contents : ""
        const parts = []
        for (const editor of orderedEditors()) {
            if (editor.selectionStart !== editor.selectionEnd)
                parts.push(qmlNoteEditor.markdownSelection(editor.textDocument,
                                                            editor.selectionStart, editor.selectionEnd))
        }
        return parts.join("\n")
    }

    function copyDocumentSelection() {
        if (wholeDocumentSelected) {
            qmlNoteEditor.copyDocumentToClipboard()
            return
        }
        const markdown = selectedDocumentMarkdown()
        if (markdown.length > 0)
            qmlNoteEditor.copyMarkdownToClipboard(markdown)
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

    function focusFollowingBlock(blockIndex, appendIfMissing) {
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
        if (!appendIfMissing)
            return
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
        if (modifiers || (event.key !== Qt.Key_Up && event.key !== Qt.Key_Down))
            return false
        const rectangle = editor.positionToRectangle(editor.cursorPosition)
        if (event.key === Qt.Key_Up) {
            if (editor.blockIndex <= 0 || rectangle.y > editor.positionToRectangle(0).y + 0.5)
                return false
            focusPrecedingBlock(editor.blockIndex)
        } else {
            const last = editor.positionToRectangle(editor.length)
            if (rectangle.y < last.y - 0.5)
                return false
            focusFollowingBlock(editor.blockIndex, false)
        }
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

    function handleHeadingShortcut(event, editor) {
        const modifiers = event.modifiers
        if (!(modifiers & Qt.ControlModifier) || modifiers & (Qt.ShiftModifier | Qt.AltModifier | Qt.MetaModifier)
                || event.key < Qt.Key_0 || event.key > Qt.Key_6)
            return false
        const level = event.key - Qt.Key_0
        const row = blockModel.convertTextBlockToHeading(editor.blockIndex, editor.cursorPosition, level)
        if (row < 0)
            return false
        focusBlock(row)
        return true
    }

    function inlineMarkers(event) {
        const primary = event.modifiers & (Qt.ControlModifier | Qt.MetaModifier)
        if (!primary || event.modifiers & Qt.AltModifier)
            return null
        if (event.key === Qt.Key_B && !(event.modifiers & Qt.ShiftModifier))
            return "bold"
        if (event.key === Qt.Key_I && !(event.modifiers & Qt.ShiftModifier))
            return "italic"
        if (event.key === Qt.Key_S && event.modifiers & Qt.ShiftModifier)
            return "strike"
        if (event.key === Qt.Key_QuoteLeft && !(event.modifiers & Qt.ShiftModifier))
            return "code"
        if (event.key === Qt.Key_K && !(event.modifiers & Qt.ShiftModifier))
            return "link"
        return null
    }

    function applyInlineStyle(editor, style) {
        const start = editor.selectionStart
        const end = editor.selectionEnd
        const formattedEnd = qmlNoteEditor.applyInlineFormat(editor.textDocument, start, end, style)
        if (formattedEnd < 0)
            return
        editor.select(start, formattedEnd)
        editor.commitText(false)
    }

    function openLinkEditor(editor) {
        if (!editor || editor.textFormat !== TextEdit.MarkdownText)
            return
        const info = qmlNoteEditor.linkInfo(editor.textDocument,
                                            editor.selectionStart,
                                            editor.selectionEnd)
        if (!info.valid)
            return
        editor.select(info.start, info.end)
        linkEditor.openFor(editor, info.start, info.end, info.href)
    }

    function handleInlineFormatting(event, editor) {
        const style = inlineMarkers(event)
        if (!style)
            return false
        if (style === "link") {
            if (!editor || editor.textFormat !== TextEdit.MarkdownText)
                return false
            const selected = orderedEditors().filter(candidate => candidate.selectionStart !== candidate.selectionEnd)
            openLinkEditor(selected.length === 1 ? selected[0] : editor)
            return true
        }
        const selected = orderedEditors().filter(candidate => candidate.selectionStart !== candidate.selectionEnd)
        if (selected.length > 1) {
            for (const candidate of selected)
                applyInlineStyle(candidate, style)
            refreshSelectionState()
        } else {
            applyInlineStyle(editor, style)
        }
        return true
    }

    Popup {
        id: linkEditor
        parent: Overlay.overlay
        modal: false
        focus: !hoverMode
        padding: 8
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: Math.min(420, Math.max(240, parent ? parent.width - 16 : 420))

        property var editor: null
        property int selectionStart: 0
        property int selectionEnd: 0
        property bool hadLink: false
        property bool hoverMode: false
        property bool pointerInside: false

        Timer {
            id: hoverLinkCloseTimer
            interval: 250
            onTriggered: {
                if (!linkEditor.hoverMode || linkEditor.pointerInside)
                    return
                if (linkEditor.editor && linkEditor.editor.linkHoverActive)
                    return
                linkEditor.close()
            }
        }

        function openFor(target, start, end, href, activate) {
            hoverLinkCloseTimer.stop()
            editor = target
            selectionStart = start
            selectionEnd = end
            urlField.text = href || ""
            hadLink = urlField.text.length > 0
            hoverMode = activate === false
            const rectangle = target.positionToRectangle(start)
            const point = target.mapToItem(Overlay.overlay, rectangle.x, rectangle.y)
            x = Math.max(8, Math.min(Overlay.overlay.width - width - 8, point.x))
            y = Math.max(8, point.y - implicitHeight - 6)
            open()
            if (!hoverMode) {
                Qt.callLater(function() {
                    urlField.forceActiveFocus()
                    urlField.selectAll()
                })
            }
        }

        function scheduleHoverClose(target) {
            if (hoverMode && editor === target)
                hoverLinkCloseTimer.restart()
        }

        function applyLink(href) {
            if (!editor)
                return
            const end = qmlNoteEditor.setLink(editor.textDocument,
                                              selectionStart,
                                              selectionEnd,
                                              href.trim())
            if (end >= 0) {
                editor.select(selectionStart, end)
                editor.commitText(false)
            }
            close()
            editor.forceActiveFocus()
        }

        onClosed: {
            hoverLinkCloseTimer.stop()
            pointerInside = false
            if (!hoverMode && editor)
                editor.forceActiveFocus()
            hoverMode = false
        }

        contentItem: RowLayout {
            spacing: 6

            HoverHandler {
                onHoveredChanged: {
                    linkEditor.pointerInside = hovered
                    if (hovered)
                        hoverLinkCloseTimer.stop()
                    else if (linkEditor.hoverMode)
                        hoverLinkCloseTimer.restart()
                }
            }

            TextField {
                id: urlField
                objectName: "noteLinkUrlField"
                Layout.fillWidth: true
                placeholderText: qsTr("Paste or type a link")
                selectByMouse: true
                onActiveFocusChanged: {
                    if (activeFocus) {
                        linkEditor.hoverMode = false
                        hoverLinkCloseTimer.stop()
                    }
                }
                onAccepted: linkEditor.applyLink(text)
            }
            Button {
                text: qsTr("Apply")
                onClicked: linkEditor.applyLink(urlField.text)
            }
            ToolButton {
                text: qsTr("Remove")
                enabled: linkEditor.hadLink
                onClicked: linkEditor.applyLink("")
            }
        }
    }

    delegate: FocusScope {
        id: blockDelegate
        required property int index
        required property int blockType
        required property string blockText
        required property var items
        required property var checkedItems
        required property var itemIndents
        required property var itemTypes
        required property int headingLevel
        required property var table
        required property string url
        required property string alt
        required property url previewUrl
        property alias item: blockLoader.item
        width: Math.max(0, root.width - root.scrollBarInset)
        height: blockLoader.height

        Loader {
            id: blockLoader
            property int index: blockDelegate.index
            property int blockType: blockDelegate.blockType
            property string blockText: blockDelegate.blockText
            property var items: blockDelegate.items
            property var checkedItems: blockDelegate.checkedItems
            property var itemIndents: blockDelegate.itemIndents
            property var itemTypes: blockDelegate.itemTypes
            property int headingLevel: blockDelegate.headingLevel
            property var table: blockDelegate.table
            property string url: blockDelegate.url
            property string alt: blockDelegate.alt
            property url previewUrl: blockDelegate.previewUrl
            x: root.editorInset
            width: Math.max(0, blockDelegate.width - 2 * root.editorInset)
            height: item ? item.implicitHeight : 0
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
                           : blockType === 4 ? imageEditor
                           : blockType === 6 ? headingEditor : textEditor
        }
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
        readonly property bool renderedMarkdown: textFormat === TextEdit.MarkdownText
        readonly property bool linkHoverActive: editorMouseArea.containsMouse
                                                && editorMouseArea.hoveredRenderedLinkInfo !== null
        property bool primaryModifierDown: false
        property string sourceText: ""
        property bool syncingSourceText: false
        property bool sourceTextPending: false
        property string observedPlainText: ""
        property bool observedPlainTextInitialized: false
        wrapMode: TextEdit.Wrap
        verticalAlignment: TextEdit.AlignTop
        selectByMouse: true
        persistentSelection: true
        background: null
        leftPadding: 4
        rightPadding: 4
        topPadding: 0
        bottomPadding: 0
        function currentPlainText() {
            return getText(0, length)
        }

        function rememberPlainText() {
            observedPlainText = currentPlainText()
            observedPlainTextInitialized = true
        }

        function synchronizeSourceText(force) {
            if (activeFocus) {
                sourceTextPending = true
                return
            }
            sourceTextPending = false
            if (!force && text === sourceText) {
                rememberPlainText()
                return
            }
            syncingSourceText = true
            text = sourceText
            syncingSourceText = false
            rememberPlainText()
        }

        function shouldCommitTextChange(allowed) {
            if (syncingSourceText)
                return false
            const current = currentPlainText()
            if (!observedPlainTextInitialized) {
                observedPlainText = current
                observedPlainTextInitialized = true
                return false
            }
            const contentChanged = current !== observedPlainText
            observedPlainText = current
            return Boolean(allowed) && contentChanged
        }

        onSourceTextChanged: synchronizeSourceText()
        onActiveFocusChanged: {
            if (activeFocus) {
                root.activeEditor = this
                rememberPlainText()
            } else if (sourceTextPending) {
                synchronizeSourceText()
            }
        }
        Component.onCompleted: {
            synchronizeSourceText(true)
            root.registerEditor(blockArea)
            qmlNoteEditor.registerTextDocument(textDocument, titleDocument)
            rememberPlainText()
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
            function onTextChanged() {
                spellRefresh.restart()
                plainLinkHoverCanvas.requestPaint()
            }
            function onSelectedTextChanged() {
                if (!root.mouseSelectionActive)
                    selectionStateRefresh.restart()
            }
        }

        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Control || event.key === Qt.Key_Meta) {
                primaryModifierDown = true
                editorMouseArea.refreshPlainLinkHover(event.modifiers)
                plainLinkHoverCanvas.requestPaint()
            }
            if (blockArea.handleLinkSpaceExit(event)) {
                event.accepted = true
            } else if ((event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace)
                    && root.deleteStructuredSelection()) {
                event.accepted = true
            } else if (root.handleInlineFormatting(event, blockArea)) {
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

        Keys.onReleased: function(event) {
            if (event.key === Qt.Key_Control || event.key === Qt.Key_Meta) {
                primaryModifierDown = false
                editorMouseArea.refreshPlainLinkHover(event.modifiers)
                plainLinkHoverCanvas.requestPaint()
            }
        }

        function handleLinkSpaceExit(event) {
            if (!renderedMarkdown || event.key !== Qt.Key_Space
                    || event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier)
                    || selectionStart !== selectionEnd || cursorPosition <= 0
                    || getText(cursorPosition - 1, cursorPosition) !== " ") {
                return false
            }

            const position = cursorPosition
            const info = qmlNoteEditor.linkInfo(textDocument, position - 1, position)
            if (!info.valid || !info.href || info.end !== position)
                return false

            const result = qmlNoteEditor.setLink(textDocument, position - 1, position, "")
            if (result < 0)
                return false

            cursorPosition = position
            commitText(false)
            return true
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

        function plainLinkInfoAtPosition(position) {
            const source = text
            let match
            const markdownLink = /\[[^\]]*\]\(([^)\s]+)\)/g
            while ((match = markdownLink.exec(source)) !== null) {
                const end = match.index + match[0].length
                if (position >= match.index && position < end)
                    return { href: match[1], start: match.index, end: end }
            }
            const rawUrl = /(?:(?:https?|ftp):\/\/|www\.)[^\s<>{}\[\]"']+/gi
            while ((match = rawUrl.exec(source)) !== null) {
                let visibleUrl = match[0]
                while (visibleUrl.length > 0 && ".,;:!?".indexOf(visibleUrl[visibleUrl.length - 1]) >= 0)
                    visibleUrl = visibleUrl.slice(0, -1)
                const end = match.index + visibleUrl.length
                if (position >= match.index && position < end) {
                    return {
                        href: visibleUrl.indexOf("www.") === 0 ? "https://" + visibleUrl : visibleUrl,
                        start: match.index,
                        end: end
                    }
                }
            }
            return null
        }

        function plainLinkAtPosition(position) {
            const info = plainLinkInfoAtPosition(position)
            return info ? info.href : ""
        }

        Timer {
            id: renderedLinkHoverTimer
            interval: 700
            onTriggered: {
                const info = editorMouseArea.hoveredRenderedLinkInfo
                if (!blockArea.renderedMarkdown || !editorMouseArea.containsMouse || !info)
                    return
                if (linkEditor.visible && !linkEditor.hoverMode)
                    return
                linkEditor.openFor(blockArea, info.start, info.end, info.href, false)
            }
        }

        Timer {
            id: modifierStatePoll
            interval: 50
            repeat: true
            running: editorMouseArea.containsMouse && !blockArea.renderedMarkdown
            onTriggered: {
                const pressed = qmlNoteEditor.primaryModifierPressed()
                if (blockArea.primaryModifierDown === pressed)
                    return
                blockArea.primaryModifierDown = pressed
                editorMouseArea.refreshPlainLinkHover(pressed ? Qt.ControlModifier : Qt.NoModifier)
                plainLinkHoverCanvas.requestPaint()
            }
        }

        MouseArea {
            id: editorMouseArea
            anchors.fill: parent
            z: 20
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            hoverEnabled: true
            preventStealing: true
            cursorShape: (blockArea.renderedMarkdown && hoveredRenderedLink.length > 0)
                         || (!blockArea.renderedMarkdown && blockArea.primaryModifierDown
                             && hoveredPlainLinkInfo !== null)
                         ? Qt.PointingHandCursor : Qt.IBeamCursor
            property bool selecting: false
            property bool selectionMoved: false
            property real hoverX: -1
            property real hoverY: -1
            property string hoveredRenderedLink: ""
            property var hoveredRenderedLinkInfo: null
            property var hoveredPlainLinkInfo: null

            function clearPlainLinkHover() {
                renderedLinkHoverTimer.stop()
                hoveredRenderedLink = ""
                hoveredRenderedLinkInfo = null
                hoveredPlainLinkInfo = null
                plainLinkHoverCanvas.requestPaint()
            }

            function refreshPlainLinkHover(modifiers) {
                if (hoverX < 0 || hoverY < 0) {
                    clearPlainLinkHover()
                    return
                }
                if (blockArea.renderedMarkdown) {
                    hoveredPlainLinkInfo = null
                    const href = blockArea.linkAt(hoverX, hoverY)
                    const position = blockArea.positionAt(hoverX, hoverY)
                    const info = href.length > 0
                            ? qmlNoteEditor.linkInfo(blockArea.textDocument, position, position)
                            : null
                    const changed = !hoveredRenderedLinkInfo || !info
                            || hoveredRenderedLinkInfo.start !== info.start
                            || hoveredRenderedLinkInfo.end !== info.end
                            || hoveredRenderedLinkInfo.href !== info.href
                    hoveredRenderedLink = href
                    hoveredRenderedLinkInfo = info && info.valid ? info : null
                    if (hoveredRenderedLinkInfo && changed)
                        renderedLinkHoverTimer.restart()
                    else if (!hoveredRenderedLinkInfo)
                        renderedLinkHoverTimer.stop()
                    plainLinkHoverCanvas.requestPaint()
                    return
                }

                renderedLinkHoverTimer.stop()
                hoveredRenderedLink = ""
                hoveredRenderedLinkInfo = null
                const primaryModifier = blockArea.primaryModifierDown
                        || Boolean(modifiers & (Qt.ControlModifier | Qt.MetaModifier))
                if (!primaryModifier) {
                    clearPlainLinkHover()
                    return
                }
                const position = blockArea.positionAt(hoverX, hoverY)
                hoveredPlainLinkInfo = blockArea.plainLinkInfoAtPosition(position)
                plainLinkHoverCanvas.requestPaint()
            }
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
                hoverX = mouse.x
                hoverY = mouse.y
                if (mouse.buttons === Qt.NoButton)
                    refreshPlainLinkHover(mouse.modifiers)
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
                        const position = blockArea.positionAt(mouse.x, mouse.y)
                        if (blockArea.renderedMarkdown) {
                            const link = blockArea.linkAt(mouse.x, mouse.y)
                            if (link.length > 0)
                                Qt.openUrlExternally(link)
                        } else if (mouse.modifiers & (Qt.ControlModifier | Qt.MetaModifier)) {
                            const link = blockArea.plainLinkAtPosition(position)
                            if (link.length > 0)
                                Qt.openUrlExternally(link)
                        }
                    }
                    selecting = false
                    root.mouseSelectionActive = false
                    selectionStateRefresh.restart()
                    root.selectionAnchorEditor = null
                }
            }
            onExited: {
                hoverX = -1
                hoverY = -1
                clearPlainLinkHover()
                linkEditor.scheduleHoverClose(blockArea)
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
            id: plainLinkHoverCanvas
            anchors.fill: parent
            z: 15
            visible: !blockArea.renderedMarkdown && blockArea.primaryModifierDown
                     && editorMouseArea.hoveredPlainLinkInfo !== null
            onVisibleChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                if (!visible || !editorMouseArea.hoveredPlainLinkInfo)
                    return

                const info = editorMouseArea.hoveredPlainLinkInfo
                ctx.strokeStyle = blockArea.palette.link
                ctx.lineWidth = 1.5
                let segmentLeft = -1
                let segmentY = 0
                for (let position = info.start; position < info.end; ++position) {
                    const current = blockArea.positionToRectangle(position)
                    const next = blockArea.positionToRectangle(position + 1)
                    let right = next.y === current.y && next.x > current.x
                            ? next.x
                            : current.x + Math.max(current.width, editorFontMetrics.averageCharacterWidth)
                    if (segmentLeft < 0) {
                        segmentLeft = current.x
                        segmentY = current.y + current.height - 1
                    }
                    if (next.y !== current.y || position === info.end - 1) {
                        ctx.beginPath()
                        ctx.moveTo(segmentLeft, segmentY)
                        ctx.lineTo(right, segmentY)
                        ctx.stroke()
                        segmentLeft = -1
                    }
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
            id: textCell
            property var block: parent
            titleDocument: block.index === 0
            blockIndex: block.index
            width: block.width
            sourceText: root.blockModel && root.blockModel.markdown
                        ? root.markdownForRendering(block.blockText) : block.blockText
            keyHandler: function(event) { return root.handleHeadingShortcut(event, textCell) }
            commitText: function() {
                const value = root.blockModel && root.blockModel.markdown
                    ? qmlNoteEditor.markdownText(textDocument) : text
                root.blockModel.setBlockText(block.index, value)
            }
            textFormat: root.blockModel && root.blockModel.markdown ? TextEdit.MarkdownText : TextEdit.PlainText
            onTextChanged: if (shouldCommitTextChange(activeFocus)) commitText(true)
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
    }

    Component {
        id: headingEditor
        BlockTextArea {
            id: headingCell
            property var block: parent
            blockIndex: block.index
            width: block.width
            sourceText: root.markdownForRendering(block.blockText)
            font.bold: true
            font.pointSize: Application.font.pointSize * (block.headingLevel === 1 ? 1.7
                            : block.headingLevel === 2 ? 1.5
                            : block.headingLevel === 3 ? 1.3
                            : block.headingLevel === 4 ? 1.15
                            : block.headingLevel === 5 ? 1.0 : 0.9)
            keyHandler: function(event) { return root.handleHeadingShortcut(event, headingCell) }
            commitText: function() {
                root.blockModel.setBlockText(
                    block.index, qmlNoteEditor.markdownText(textDocument))
            }
            textFormat: TextEdit.MarkdownText
            onTextChanged: if (shouldCommitTextChange(activeFocus)) commitText(true)
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
                        sourceText: root.markdownForRendering(itemText)
                        keyHandler: function(event) { return checkRoot.handleItemKey(event, checkCell, checkRow.index) }
                        commitText: function() {
                            root.blockModel.setListItem(
                                checkRoot.block.index,
                                index,
                                qmlNoteEditor.markdownText(textDocument))
                        }
                        textFormat: TextEdit.MarkdownText
                        onTextChanged: if (shouldCommitTextChange(activeFocus && !checkRoot.syncingItems))
                                           commitText(true)
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
                    sourceText: cellText
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
                    commitText: function() {
                        root.blockModel.setTableCell(tableRoot.block.index, index, text)
                    }
                    onTextChanged: if (shouldCommitTextChange(activeFocus)) commitText(true)
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
                    sourceText: imageRoot.block.alt
                    commitText: function() { root.blockModel.setImageAlt(imageRoot.block.index, text) }
                    onTextChanged: if (shouldCommitTextChange(activeFocus)) commitText(true)
                }
                BlockTextArea {
                    blockIndex: imageRoot.block.index
                    Layout.fillWidth: true
                    placeholderText: qsTr("Image URL")
                    sourceText: imageRoot.block.url
                    commitText: function() { root.blockModel.setImageUrl(imageRoot.block.index, text) }
                    onTextChanged: if (shouldCommitTextChange(activeFocus)) commitText(true)
                }
            }
        }
    }
}
