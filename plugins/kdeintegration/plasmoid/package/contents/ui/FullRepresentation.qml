/*
    SPDX-License-Identifier: GPL-3.0-only
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid

PlasmaExtras.Representation {
    id: root

    required property PlasmoidItem plasmoidItem
    required property var notesModel

    collapseMarginsHint: true
    implicitWidth: Kirigami.Units.gridUnit * 18
    implicitHeight: Kirigami.Units.gridUnit * 22

    header: PlasmaExtras.BasicPlasmoidHeading {
        leftPadding: mirrored ? 0 : Kirigami.Units.smallSpacing
        rightPadding: mirrored ? Kirigami.Units.smallSpacing : 0

        PlasmaComponents3.ToolButton {
            icon.name: "list-add-symbolic"
            text: qsTr("New Note")
            display: PlasmaComponents3.AbstractButton.IconOnly
            onClicked: {
                root.notesModel.createNote();
                root.plasmoidItem.expanded = false;
            }
            PlasmaComponents3.ToolTip.text: text
            PlasmaComponents3.ToolTip.visible: hovered
        }
    }

    PlasmaComponents3.ScrollView {
        anchors.fill: parent

        ListView {
            id: notesView

            clip: true
            model: root.notesModel

            delegate: PlasmaComponents3.ItemDelegate {
                id: noteDelegate

                required property int index
                required property string title
                required property string storageId

                width: ListView.view.width
                text: title.length > 0 ? title : qsTr("Untitled Note")
                icon.name: "text-x-generic"
                Accessible.description: qsTr("Stored in %1").arg(storageId)

                onClicked: {
                    root.notesModel.openNote(index);
                    root.plasmoidItem.expanded = false;
                }
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: parent
                width: Math.max(0, parent.width - Kirigami.Units.gridUnit * 2)
                visible: notesView.count === 0 && !root.notesModel.loading
                iconName: root.notesModel.available ? "edit-none" : "network-disconnect"
                text: root.notesModel.available
                    ? qsTr("No notes yet")
                    : qsTr("QtNote is not running")
                helpfulAction: newNoteAction
            }

            PlasmaComponents3.BusyIndicator {
                anchors.centerIn: parent
                running: root.notesModel.loading
                visible: running
            }
        }
    }

    QQC2.Action {
        id: newNoteAction
        text: qsTr("Create a Note")
        icon.name: "list-add"
        onTriggered: root.notesModel.createNote()
    }
}
