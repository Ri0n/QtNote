/*
    SPDX-License-Identifier: GPL-3.0-only
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window
import org.kde.kirigami as Kirigami
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

    readonly property bool hasFilter: root.notesModel.query.length > 0
    property int focusAttempts: 0

    function focusFilter() {
        focusAttempts = 0;
        filterFocusTimer.restart();
    }

    Timer {
        id: filterFocusTimer

        interval: 80
        repeat: true
        onTriggered: {
            ++root.focusAttempts;
            root.forceActiveFocus(Qt.PopupFocusReason);
            filterField.forceActiveFocus(Qt.PopupFocusReason);
            if (filterField.contentItem) {
                filterField.contentItem.forceActiveFocus(Qt.PopupFocusReason);
            }

            if (filterField.activeFocus || root.focusAttempts >= 10) {
                stop();
            }
        }
    }

    Connections {
        target: root.plasmoidItem

        function onExpandedChanged() {
            if (root.plasmoidItem.expanded) {
                root.focusFilter();
            } else {
                filterFocusTimer.stop();
            }
        }
    }

    header: PlasmaExtras.PlasmoidHeading {
        visible: true

        contentItem: RowLayout {
            spacing: Kirigami.Units.smallSpacing

            PlasmaExtras.SearchField {
                id: filterField
                Layout.fillWidth: true
                placeholderText: qsTr("Search notes")
                onTextChanged: {
                    notesView.currentIndex = 0;
                    root.notesModel.query = text;
                }
                onAccepted: if (notesView.currentIndex >= 0) {
                    root.notesModel.openNote(notesView.currentIndex, root.Window.window);
                }
            }
        }
    }

    PlasmaComponents3.ScrollView {
        anchors.fill: parent

        ListView {
            id: notesView

            clip: true
            model: root.notesModel

            function maybeLoadMore() {
                if (root.notesModel.hasMore
                    && !root.notesModel.loading
                    && !root.notesModel.loadingMore
                    && contentY + height >= contentHeight - Kirigami.Units.gridUnit * 4) {
                    root.notesModel.loadMore();
                }
            }

            onContentYChanged: maybeLoadMore()
            onContentHeightChanged: maybeLoadMore()
            onHeightChanged: maybeLoadMore()
            onCountChanged: maybeLoadMore()

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
                    root.notesModel.openNote(index, root.Window.window);
                    root.plasmoidItem.expanded = false;
                }
            }

            footer: Item {
                implicitWidth: footerBusyIndicator.implicitWidth
                height: root.notesModel.loadingMore ? Kirigami.Units.gridUnit * 2 : 0

                PlasmaComponents3.BusyIndicator {
                    id: footerBusyIndicator

                    x: Math.max(0, (notesView.width - width) / 2)
                    anchors.verticalCenter: parent.verticalCenter
                    running: root.notesModel.loadingMore
                    visible: running
                }
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: parent
                width: Math.max(0, parent.width - Kirigami.Units.gridUnit * 2)
                visible: notesView.count === 0 && !root.notesModel.loading && !root.notesModel.loadingMore
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
