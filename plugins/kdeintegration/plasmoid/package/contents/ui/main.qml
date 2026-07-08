/*
    SPDX-License-Identifier: GPL-3.0-only
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import plasma.applet.com.github.ri0n.qtnote 1.0 as QtNote

PlasmoidItem {
    id: root

    switchWidth: Kirigami.Units.gridUnit * 16
    switchHeight: Kirigami.Units.gridUnit * 12

    Plasmoid.icon: "qtnote-symbolic"
    Plasmoid.status: notesModel.available
        ? PlasmaCore.Types.ActiveStatus
        : PlasmaCore.Types.PassiveStatus

    toolTipMainText: qsTr("Notes")
    toolTipSubText: notesModel.available
        ? notesModel.count === 1
            ? qsTr("1 recent note")
            : notesModel.hasMore
                ? qsTr("%1+ recent notes").arg(notesModel.count)
                : qsTr("%1 recent notes").arg(notesModel.count)
        : qsTr("QtNote is not running")

    onExpandedChanged: {
        if (root.expanded) {
            notesModel.refresh();
        }
    }

    QtNote.NotesModel {
        id: notesModel
    }

    compactRepresentation: CompactRepresentation {
        plasmoidItem: root
        notesModel: notesModel
    }

    fullRepresentation: FullRepresentation {
        id: fullRepresentation

        plasmoidItem: root
        notesModel: notesModel
        newNoteAction: newNoteAction
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            id: newNoteAction
            text: qsTr("New Note")
            icon.name: "list-add-symbolic"
            priority: PlasmaCore.Action.HighPriority
            onTriggered: {
                notesModel.createNote(root.Window.window);
                root.expanded = false;
            }
        },
        PlasmaCore.Action {
            text: qsTr("Configure QtNote...")
            icon.name: "configure"
            onTriggered: notesModel.showOptions(root.Window.window)
        },
        PlasmaCore.Action {
            text: qsTr("Note Manager")
            icon.name: "view-list-details"
            onTriggered: notesModel.showNoteManager(root.Window.window)
        },
        PlasmaCore.Action {
            text: qsTr("About QtNote")
            icon.name: "help-about"
            onTriggered: notesModel.showAbout(root.Window.window)
        },
        PlasmaCore.Action {
            text: qsTr("Close QtNote")
            icon.name: "window-close"
            visible: notesModel.available
            onTriggered: notesModel.quit()
        }
    ]
}
