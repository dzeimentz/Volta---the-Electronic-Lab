// Bedienpult I (docs/ui-spec.md, Bedienpult): Batterie mit Zellenwahl und Hauptschalter, Potentiometer 1 und 2 mit
// Lichtring, Schalter (Kippschalter), Lautsprecher. Die blaue Pultflaeche schliesst die Pultfelder ein; jedes Bauteil
// steht mit seinen Feldern in einem eigenen Rahmen (Lage aus Lab.groups). Ausgeblendete Bauteile (Stufenanpassung)
// fehlen in groups. Alles hier sind Bedienhandlungen (kein Verlauf).
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: desk
    required property var controller
    property var groups: []        // [{name, left, right, ...}] in Koordinaten der Arbeitsflaeche
    property real offsetX: 0       // Pult beginnt links vor der Arbeitsflaeche
    property real fieldsBottom: 0  // Unterkante der Pultfelder (Rahmen schliessen sie ein)
    property string highlightGroup: ""
    signal groupTapped(string name)

    color: Theme.desk
    radius: 10

    function group(name) {
        for (let g = 0; g < groups.length; ++g) if (groups[g].name === name) return groups[g]
        return null
    }
    function centerOf(name) { const g = group(name); return g ? offsetX + (g.left + g.right) / 2 : 0 }
    readonly property color frameColor: "#8FA6BC"

    // Rahmen je Bauteil: Bedienelement oben, Felder unten; Tippen hebt die Felder hervor
    Repeater {
        model: desk.groups
        Rectangle {
            required property var modelData
            x: desk.offsetX + modelData.left
            y: 6
            width: modelData.right - modelData.left
            height: desk.fieldsBottom - y
            radius: 10
            color: Qt.lighter(Theme.desk, 1.12)
            border.color: desk.highlightGroup === modelData.name ? Theme.signalOrange : desk.frameColor
            border.width: 2
            TapHandler { onTapped: desk.groupTapped(modelData.name) }
        }
    }

    // Batterie: Zellenwahl und Hauptschalter
    Column {
        visible: desk.group("batterie") !== null
        x: desk.centerOf("batterie") - width / 2
        y: 12
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
        visible: desk.group("p1") !== null
        x: desk.centerOf("p1") - width / 2
        y: 8
        label: qsTr("Potentiometer 1")
        value: desk.controller.pot1
        onMoved: (v) => desk.controller.pot1 = v
    }
    KnobRing {
        objectName: "knobP2"
        visible: desk.group("p2") !== null
        x: desk.centerOf("p2") - width / 2
        y: 8
        label: qsTr("Potentiometer 2")
        value: desk.controller.pot2
        onMoved: (v) => desk.controller.pot2 = v
    }

    // Schalter: Kippschalter (Knebel oben = ein, unten = aus), Beschriftung darunter
    Item {
        objectName: "deskSwitch"
        visible: desk.group("schalter") !== null
        x: desk.centerOf("schalter") - width / 2
        y: 12
        width: 80
        height: 108
        readonly property bool on: desk.controller.deskSwitch
        Rectangle { // Metallplatte
            id: plate
            anchors.horizontalCenter: parent.horizontalCenter
            width: 46; height: 72; radius: 9
            color: "#3A434D"
            border.color: "#8E99A4"; border.width: 2
            Rectangle { // Drehpunkt
                anchors.centerIn: parent
                width: 18; height: 18; radius: 9
                color: "#6B7580"
            }
            Rectangle { // Knebel
                anchors.horizontalCenter: parent.horizontalCenter
                width: 14; height: 32; radius: 7
                y: parent.parent.on ? 4 : parent.height - height - 4
                color: "#D5DCE3"
                border.color: "#8E99A4"
                Behavior on y { enabled: !Theme.reduceMotion; NumberAnimation { duration: 90 } }
            }
        }
        // Stellungen wie auf einem echten Pult beschriftet (nie nur ueber die Lage)
        Text {
            anchors.left: plate.right; anchors.leftMargin: 3; y: 4
            text: qsTr("ein"); color: parent.on ? "white" : "#8FA6BC"
            font.pixelSize: Math.round((11) * Theme.fontScale); font.bold: parent.on
        }
        Text {
            anchors.left: plate.right; anchors.leftMargin: 3; y: plate.height - height - 4
            text: qsTr("aus"); color: parent.on ? "#8FA6BC" : "white"
            font.pixelSize: Math.round((11) * Theme.fontScale); font.bold: !parent.on
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: plate.bottom
            anchors.topMargin: 6
            text: qsTr("Schalter")
            color: "white"
            font.pixelSize: Math.round((13) * Theme.fontScale)
            font.bold: true
        }
        TapHandler { onTapped: desk.controller.deskSwitch = !desk.controller.deskSwitch }
        Accessible.role: Accessible.CheckBox
        Accessible.name: qsTr("Schalter")
        Accessible.checkable: true
        Accessible.checked: on
        Accessible.onPressAction: desk.controller.deskSwitch = !desk.controller.deskSwitch
    }

    // Lautsprecher mit Leuchtring (Klanganzeige)
    Column {
        visible: desk.group("lautsprecher") !== null
        x: desk.centerOf("lautsprecher") - width / 2
        y: 12
        spacing: 2
        SpeakerRing {
            anchors.horizontalCenter: parent.horizontalCenter
            controller: desk.controller
        }
        Label { text: qsTr("Lautsprecher"); color: "white"; font.pixelSize: Math.round((12) * Theme.fontScale); anchors.horizontalCenter: parent.horizontalCenter }
    }
}
