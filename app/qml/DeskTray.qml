// Geraetefach (Pult nach Bedarf, Konzept Volta 2, 5.5): alle Pultteile des Bandes, nach Gruppen mit Bildzeichen und
// Namen (Farbe nie allein). Antippen setzt ein Teil ins Pult (Befehl); was schon im Pult steht oder erst in einer
// spaeteren Stufe frei wird, ist gekennzeichnet. Herausnehmen geht ueber das »?« des Pultteils (DeskPartInfo).
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: tray
    property var controller: null

    function title(part) {
        switch (part) {
        case "batterie": return qsTr("Batterie")
        case "batterie2": return qsTr("Batterie 2")
        case "p1": return qsTr("Potentiometer 1")
        case "p2": return qsTr("Potentiometer 2")
        case "schalter": return qsTr("Schalter")
        case "lautsprecher": return qsTr("Lautsprecher")
        }
        return part
    }
    function groupOf(part) {
        return part.startsWith("batterie") ? "quellen" : part === "lautsprecher" ? "hoeren" : "bedienen"
    }
    readonly property var groups: [
        {key: "quellen", name: qsTr("Quellen"), sign: "+", color: "#C62828"},
        {key: "bedienen", name: qsTr("Bedienen"), sign: "\u21BB", color: "#2873B0"},
        {key: "hoeren", name: qsTr("Hören"), sign: "\u266A", color: "#6A3FA0"}
    ]

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(parent ? parent.width - 32 : 460, 460)
    height: Math.min(parent ? parent.height - 32 : 600, list.implicitHeight + header.height + 28)
    padding: 0
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    Rectangle {
        id: header
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: Theme.control + 8
        radius: 12
        color: Theme.desk
        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 12; color: Theme.desk }
        Label {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Gerätefach")
            color: "white"
            font.pixelSize: Math.round(18 * Theme.fontScale)
            font.bold: true
        }
        ToolButton {
            anchors.right: parent.right
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.control; height: Theme.control
            contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: "white"; size: 18 } }
            onClicked: tray.close()
            Accessible.name: qsTr("Schließen")
        }
    }

    Flickable {
        anchors { left: parent.left; right: parent.right; top: header.bottom; bottom: parent.bottom; margins: 14 }
        contentHeight: list.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: list
            width: parent.width
            spacing: 8
            Repeater {
                model: tray.groups
                Column {
                    id: grp
                    required property var modelData
                    readonly property var items: tray.controller ? tray.controller.deskTray.filter(
                                                     t => tray.groupOf(t.part) === modelData.key) : []
                    visible: items.length > 0
                    width: list.width
                    spacing: 6
                    Row { // Gruppe: Bildzeichen in der Gruppenfarbe und Name
                        spacing: 8
                        Rectangle {
                            width: 26; height: 26; radius: 13
                            color: grp.modelData.color
                            Text { anchors.centerIn: parent; text: grp.modelData.sign; color: "white"; font.pixelSize: 15; font.bold: true }
                        }
                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            text: grp.modelData.name
                            font.pixelSize: Math.round(15 * Theme.fontScale)
                            font.bold: true
                            color: Theme.navy
                        }
                    }
                    Repeater {
                        model: grp.items
                        Button {
                            required property var modelData
                            objectName: "deskTray_" + modelData.part
                            width: grp.width
                            height: Theme.control
                            enabled: !modelData.present && !modelData.locked
                            text: tray.title(modelData.part) + "  –  "
                                  + (modelData.present ? qsTr("steht im Pult")
                                     : modelData.locked ? qsTr("ab Stufe %1").arg(modelData.stage)
                                     : qsTr("einsetzen"))
                            onClicked: if (tray.controller.addDeskPart(modelData.part)) tray.close()
                        }
                    }
                }
            }
        }
    }
}
