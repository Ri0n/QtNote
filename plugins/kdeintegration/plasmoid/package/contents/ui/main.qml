/*
    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

pragma ComponentBehavior: Bound

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

PlasmoidItem {
    id: root

    property alias openNoteAction: openNoteAction

    switchWidth: Kirigami.Units.gridUnit * 15
    switchHeight: Kirigami.Units.gridUnit * 10

    // Представление для компактного режима (значок в трее)
    compactRepresentation: CompactRepresentation {
        plasmoidItem: root
        openNoteAction: root.openNoteAction
    }

    // Полное представление с доступом к списку заметок
    fullRepresentation: FullRepresentation {
        plasmoidItem: root

        ListView {
            id: noteListView
            width: parent.width
            height: parent.height

            // Модель для списка заметок
            model: noteModel

            delegate: Item {
                width: noteListView.width
                height: Kirigami.Units.gridUnit * 3

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Text {
                        text: modelData.title
                        font.weight: Font.Bold
                    }

                    Text {
                        text: modelData.summary
                    }

                    PlasmaCore.Action {
                        text: i18n("Open")
                        onTriggered: root.openNoteAction.trigger(modelData.id)
                    }
                }
            }
        }
    }

    Plasmoid.icon: "note-icon"
    toolTipMainText: i18n("Notes")
    toolTipSubText: i18n("Manage and view notes")

    // Действия для плазмоида
    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            id: openNoteAction
            text: i18n("Open Note")
            icon.name: "document-open"
            onTriggered: modelData => openNoteById(modelData.id)
        }
    ]

    Component.onCompleted: {
        // Любая дополнительная инициализация
    }

    function openNoteById(id) {
        // Вызов C++ функции для открытия заметки по идентификатору
        // Пример: qtnote.openNoteById(id)
    }
}
