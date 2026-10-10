// Bedienpult I (docs/ui-spec.md, Bedienpult): Batterie mit Zellenwahl und Hauptschalter, Drehregler P1 und P2 mit
// Lichtring, Pultschalter S, Lautsprecher. Jedes Element sitzt ueber seinen Feldern der Pultleiste (Positionen von
// Lab.qml). Alles hier sind Bedienhandlungen (kein Verlauf).
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: desk
    required property var controller
    // Mittelpunkte (x) der Gruppen ueber der Pultleiste
    property real batteryX
    property real p1X
    property real p2X
    property real switchX
    property real speakerX

    color: Theme.desk
    radius: 10
    height: 128

    // Batterie: angedeutete 4,5-V-Flachbatterie als Hintergrund (Gehaeuse mit Banderole, zwei Messingfahnen:
    // kurz = Plus, lang = Minus), darauf Zellenwahl und Hauptschalter
    Item {
        id: flatBattery
        x: Math.max(6, desk.batteryX - width / 2)
        y: 16
        width: 114 // Platz bis zum Drehregler P1
        height: desk.height - 22
        Rectangle { // Plus: kurze, breite Fahne (links neben der Beschriftung)
            x: 6; y: -10; width: 18; height: 12; radius: 2
            color: "#C9A227"; border.color: "#8C6E12"
        }
        Rectangle { // Minus: lange, schmale Fahne
            x: parent.width - 18; y: -12; width: 10; height: 14; radius: 2
            color: "#C9A227"; border.color: "#8C6E12"
        }
        Rectangle { // Gehaeuse
            anchors.fill: parent
            radius: 8
            color: "#2C5A85"
            border.color: "#14324F"
            border.width: 2
            Rectangle { // Banderole
                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                anchors.margins: 2
                height: parent.height * 0.40
                color: "#C7772A"
            }
        }
    }

    // Batterie: Zellenwahl und Hauptschalter
    Column {
        x: desk.batteryX - width / 2
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4
        Label { text: qsTr("Batterie"); color: "white"; font.pixelSize: Math.round((13) * Theme.fontScale); font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
        ComboBox {
            objectName: "cellsBox"
            width: 96
            height: Theme.control - 6
            // Zahlenformat der Anzeigesprache (4,5 V / 4.5 V); Neuaufbau bei Sprachwechsel ueber qsTr-Bindung
            model: [qsTr("%1 V").arg(3), qsTr("%1 V").arg((4.5).toLocaleString(Qt.locale(), "f", 1)), qsTr("%1 V").arg(9)]
            currentIndex: desk.controller.cells === 2 ? 0 : (desk.controller.cells === 3 ? 1 : 2)
            onActivated: (index) => { desk.controller.cells = [2, 3, 6][index] }
            Accessible.name: qsTr("Batteriespannung")
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 4
            Switch {
                objectName: "mainSwitch"
                checked: desk.controller.mainSwitch
                onToggled: desk.controller.mainSwitch = checked
                Accessible.name: qsTr("Hauptschalter")
            }
            // Stellung auch als Text (Farbe nie allein)
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: desk.controller.mainSwitch ? qsTr("Ein") : qsTr("Aus")
                color: "white"
                font.pixelSize: Math.round((14) * Theme.fontScale)
                font.bold: true
            }
        }
    }

    KnobRing {
        objectName: "knobP1"
        x: desk.p1X - width / 2
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -8
        label: "P1"
        value: desk.controller.pot1
        onMoved: (v) => desk.controller.pot1 = v
    }
    KnobRing {
        objectName: "knobP2"
        x: desk.p2X - width / 2
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -8
        label: "P2"
        value: desk.controller.pot2
        onMoved: (v) => desk.controller.pot2 = v
    }

    // Pultschalter S
    Column {
        x: desk.switchX - width / 2
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6
        Label { text: "S"; color: "white"; font.pixelSize: Math.round((13) * Theme.fontScale); font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
        Switch {
            objectName: "deskSwitch"
            checked: desk.controller.deskSwitch
            onToggled: desk.controller.deskSwitch = checked
            Accessible.name: qsTr("Pultschalter")
        }
    }

    // Lautsprecher mit Leuchtring (Klanganzeige)
    Column {
        x: desk.speakerX - width / 2
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2
        SpeakerRing {
            anchors.horizontalCenter: parent.horizontalCenter
            controller: desk.controller
        }
        Label { text: qsTr("Lautsprecher"); color: "white"; font.pixelSize: Math.round((12) * Theme.fontScale); anchors.horizontalCenter: parent.horizontalCenter }
    }
}
