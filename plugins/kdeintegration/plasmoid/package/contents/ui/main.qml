/*
    SPDX-License-Identifier: GPL-3.0-only
*/

pragma ComponentBehavior: Bound

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import com.github.ri0n.qtnote 1.0 as QtNote

PlasmoidItem {
    id: root

    switchWidth: Kirigami.Units.gridUnit * 16
    switchHeight: Kirigami.Units.gridUnit * 12

    Plasmoid.icon: "qtnote"
    Plasmoid.status: notesModel.available
        ? PlasmaCore.Types.ActiveStatus
        : PlasmaCore.Types.PassiveStatus

    toolTipMainText: qsTr("Notes")
    toolTipSubText: notesModel.available
        ? notesModel.count === 1
            ? qsTr("1 recent note")
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
        plasmoidItem: root
        notesModel: notesModel
    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: qsTr("Configure QtNote...")
            icon.name: "configure"
            onTriggered: notesModel.showOptions()
        },
        PlasmaCore.Action {
            text: qsTr("Note Manager")
            icon.name: "view-list-details"
            onTriggered: notesModel.showNoteManager()
        },
        PlasmaCore.Action {
            text: qsTr("About QtNote")
            icon.name: "help-about"
            onTriggered: notesModel.showAbout()
        }
    ]
}
