.pragma library

function handleKey(host, controller, event, cell) {
    const row = Math.floor(cell.index / host.columns)
    const column = cell.index % host.columns
    const modifiers = event.modifiers & (Qt.ShiftModifier | Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier)
    const emptyCell = cell.length === 0 && cell.cursorPosition === 0
    if (!modifiers && host.tableEmpty()) {
        if (event.key === Qt.Key_Backspace && cell.index === 0) {
            controller.removeTableBlock(host.block.index, true)
            return true
        }
        if (event.key === Qt.Key_Delete && cell.index + 1 === host.cellCount()) {
            controller.removeTableBlock(host.block.index, false)
            return true
        }
    }
    if (!modifiers && (event.key === Qt.Key_Backspace || event.key === Qt.Key_Delete)
            && emptyCell
            && host.rowCount() > 1 && host.rowEmpty(row)) {
        const previousRowLastCell = row > 0 ? row * host.columns - 1 : -1
        const previousRowEnd = previousRowLastCell >= 0 ? host.cellLength(previousRowLastCell) : 0
        controller.blockModel.removeTableRow(host.block.index, row)
        if (event.key === Qt.Key_Backspace && column === 0 && previousRowLastCell >= 0)
            host.focusCell(previousRowLastCell, previousRowEnd)
        else
            host.focusCell(Math.max(0, row - 1) * host.columns + Math.min(column, host.columns - 1), 0)
        return true
    }
    if (!modifiers && event.key === Qt.Key_Backspace && emptyCell) {
        if (cell.index > 0)
            host.focusCell(cell.index - 1, host.cellLength(cell.index - 1))
        else
            controller.focusPrecedingBlock(host.block.index)
        return true
    }
    if (!modifiers && event.key === Qt.Key_Left && cell.cursorPosition === 0) {
        if (cell.index > 0)
            host.focusCell(cell.index - 1, host.cellLength(cell.index - 1))
        return true
    }
    if (!modifiers && event.key === Qt.Key_Right && cell.cursorPosition === cell.length) {
        if (cell.index + 1 < host.cellCount())
            host.focusCell(cell.index + 1, 0)
        return true
    }
    if (!modifiers && event.key === Qt.Key_Down && row + 1 >= host.rowCount()) {
        controller.focusFollowingBlock(host.block.index, !event.isAutoRepeat)
        return true
    }
    if (!modifiers && (event.key === Qt.Key_Up || event.key === Qt.Key_Down)) {
        const rectangle = cell.positionToRectangle(cell.cursorPosition)
        const boundary = event.key === Qt.Key_Up
            ? rectangle.y <= cell.positionToRectangle(0).y + 0.5
            : rectangle.y >= cell.positionToRectangle(cell.length).y - 0.5
        if (boundary) {
            const target = cell.index + (event.key === Qt.Key_Up ? -host.columns : host.columns)
            if (target >= 0 && target < host.cellCount())
                host.focusCellVertically(target, rectangle.x, event.key === Qt.Key_Up)
            else if (event.key === Qt.Key_Up)
                controller.focusPrecedingBlock(host.block.index)
            else if (event.key === Qt.Key_Down)
                controller.focusFollowingBlock(host.block.index, !event.isAutoRepeat)
            return true
        }
    }
    if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
            && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))) {
        if (event.modifiers & Qt.ShiftModifier)
            return false
        if (row + 1 === host.rowCount() && host.rowCount() > 1
                && cell.length === 0 && cell.cursorPosition === 0 && host.rowEmpty(row)) {
            controller.blockModel.removeTableRow(host.block.index, row)
            controller.focusFollowingBlock(host.block.index, true)
            return true
        }
        if (cell.cursorPosition === cell.length) {
            host.insertRow(row + 1, column)
            return true
        }
    }
    if (event.key === Qt.Key_Tab
            && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))) {
        if (event.modifiers & Qt.ShiftModifier) {
            if (cell.index > 0)
                host.focusCell(cell.index - 1, 0)
        } else if (cell.index + 1 < host.cellCount()) {
            host.focusCell(cell.index + 1, 0)
        } else {
            host.insertRow(row + 1, 0)
        }
        return true
    }
    return false
}
