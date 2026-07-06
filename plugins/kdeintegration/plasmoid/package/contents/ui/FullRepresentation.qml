/*
    SPDX-License-Identifier: GPL-3.0-only
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid

PlasmaExtras.Representation {
    id: root

    required property PlasmoidItem plasmoidItem
    required property var notesModel
    required property var newNoteAction

    collapseMarginsHint: true
    implicitWidth: Kirigami.Units.gridUnit * 18
    implicitHeight: Kirigami.Units.gridUnit * 22

    readonly property bool hasFilter: filterField.text.length > 0

    header: PlasmaExtras.PlasmoidHeading {
        visible: true

        contentItem: RowLayout {
            spacing: Kirigami.Units.smallSpacing

            PlasmaExtras.SearchField {
                id: filterField
                Layout.fillWidth: true
                placeholderText: qsTr("Search notes")
                onTextChanged: notesView.currentIndex = 0
                onAccepted: if (notesView.currentIndex >= 0) {
                    const sourceIndex = notesView.model.mapToSource(notesView.model.index(notesView.currentIndex, 0));
                    root.notesModel.openNote(sourceIndex.row);
                }
            }
        }
    }

    PlasmaComponents3.ScrollView {
        anchors.fill: parent

        ListView {
            id: notesView
            readonly property var proxyModel: notesProxy

            clip: true
            model: notesProxy

            KItemModels.KSortFilterProxyModel {
                id: notesProxy
                sourceModel: root.notesModel
                filterRoleName: "title"
                filterRegularExpression: RegExp(filterField.text, "i")
            }

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
                    const sourceIndex = notesView.proxyModel.mapToSource(notesView.proxyModel.index(index, 0));
                    root.notesModel.openNote(sourceIndex.row);
                    root.plasmoidItem.expanded = false;
                }
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: parent
                width: Math.max(0, parent.width - Kirigami.Units.gridUnit * 2)
                visible: notesView.count === 0 && !root.notesModel.loading
                iconName: root.notesModel.available ? "edit-none" : "network-disconnect"
                text: root.notesModel.available
                    ? root.hasFilter
                        ? qsTr("No notes match the search")
                        : qsTr("No notes yet")
                    : qsTr("QtNote is not running")
                helpfulAction: placeholderNewNoteAction
            }

            PlasmaComponents3.BusyIndicator {
                anchors.centerIn: parent
                running: root.notesModel.loading
                visible: running
            }
        }
    }

    QQC2.Action {
        id: placeholderNewNoteAction

        text: root.newNoteAction.text
        icon.name: root.newNoteAction.icon.name
        onTriggered: root.newNoteAction.trigger()
    }
}
