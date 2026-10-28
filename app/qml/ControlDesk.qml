// Bedienpult I (docs/ui-spec.md, Bedienpult): Batterie mit Zellenwahl und Hauptschalter, Potentiometer 1 und 2 mit
// Lichtring, Schalter (Kippschalter), Lautsprecher. Die blaue Pultflaeche schliesst die Pultfelder ein; jedes Bauteil
// steht mit seinen Feldern in einem eigenen Rahmen (Lage aus Lab.groups). Ausgeblendete Bauteile (Stufenanpassung)
// fehlen in groups. Alles hier sind Bedienhandlungen (kein Verlauf). Rundes »?« oben rechts je Rahmen erklaert das
// Bauteil und seine Felder (DeskPartInfo).
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Rectangle {
    id: desk
    required property var controller
    property var groups: []        // [{name, left, right, ...}] in Koordinaten der Arbeitsflaeche
    property real offsetX: 0       // Pult beginnt links vor der Arbeitsflaeche
    property real fieldsBottom: 0  // Unterkante der Pultfelder (Rahmen schliessen sie ein)
    property real fieldsTop: 0     // Oberkante der Pultfelder (Schaltzeichen sitzen direkt darueber)
    property bool meterFrame: false // Messgeraet oben im Pult: eigener Rahmen um Anzeige und Buchsen
    property real meterLeft: 0
    property real meterWidth: 0
    property string highlightGroup: ""
    signal groupTapped(string name)
    signal infoRequested(string name)

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
            // »?«: sichtbar 24 dp, Tippflaeche 48 dp; liegt unter den Bedienelementen, damit diese Vorrang haben
            AbstractButton {
                objectName: "deskInfo_" + parent.modelData.name
                x: parent.width - 30 - (width - 24) / 2
                y: 4 - (height - 24) / 2
                width: Theme.control
                height: Theme.control
                onClicked: desk.infoRequested(parent.modelData.name)
                Accessible.name: qsTr("Was ist das?")
                background: Rectangle {
                    anchors.centerIn: parent
                    width: 24; height: 24; radius: 12
                    color: parent.down ? "#FFFFFF" : (parent.hovered ? "#E6EEF6" : "#C9D6E3")
                    border.color: "#FFFFFF"
                    border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: "?"
                        color: Theme.navy
                        font.pixelSize: 15
                        font.bold: true
                    }
                }
            }
        }
    }

    // Rahmen um das Messgeraet oben im Pult (Anzeige oben, Buchsen COM, V/Ω, A unten) wie bei den Bauteilen
    Rectangle {
        objectName: "meterDeskFrame"
        visible: desk.meterFrame
        x: desk.offsetX + desk.meterLeft - 12
        y: 6
        width: desk.meterWidth + 24
        height: desk.fieldsBottom - y
        radius: 10
        color: Qt.lighter(Theme.desk, 1.12)
        border.color: desk.frameColor
        border.width: 2
    }

    // Schaltzeichen ueber den Feldern (DIN EN 60617) mit Leitungen hinunter zu jedem Feld: Batterie (langer Strich
    // Pluspol ueber V+, kurzer dicker Minuspol ueber 0V), Potentiometer (Schleifer als Pfeil zum mittleren Feld),
    // Schalter (zeigt die Stellung), Lautsprecher
    Repeater {
        model: desk.groups.filter(g => ["batterie", "p1", "p2", "schalter", "lautsprecher"].indexOf(g.name) >= 0)
        Shape {
            id: deskSymbol
            required property var modelData
            objectName: "deskSymbol_" + modelData.name
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            readonly property real fieldTop: desk.fieldsTop
            readonly property real y0: fieldTop - 20 // Hoehe der Leitung im Schaltzeichen
            function cx(k) {
                return desk.offsetX + modelData.fieldsLeft + k * (Theme.deskFieldWidth + Theme.deskFieldGap)
                       + Theme.deskFieldWidth / 2
            }
            readonly property var paths: {
                const t = fieldTop, y = y0
                switch (modelData.name) {
                case "batterie": { // V+, 0V
                    const a = cx(0), b = cx(1), m = (a + b) / 2
                    return {s: `M${a} ${t} V${y} H${m - 4} M${m + 4} ${y} H${b} V${t} M${m - 4} ${y - 9} V${y + 9}`,
                            f: `M${m + 2} ${y - 5} H${m + 6} V${y + 5} H${m + 2} Z`, plus: m - 11}
                }
                case "p1": case "p2": { // a, s, b: Widerstand zwischen a und b, Schleifer von s
                    const a = cx(0), s = cx(1), b = cx(2)
                    return {s: `M${a} ${t} V${y} H${s - 22} M${s + 22} ${y} H${b} V${t} M${s - 22} ${y - 6} H${s + 22} V${y + 6} H${s - 22} Z M${s} ${t} V${y + 11}`,
                            f: `M${s} ${y + 7} L${s - 4} ${y + 14} L${s + 4} ${y + 14} Z`}
                }
                case "schalter": { // Schliesser; geschlossen, wenn der Schalter ein ist
                    const a = cx(0), b = cx(1), l = a + 18, r = b - 18
                    const end = desk.controller.deskSwitch ? `L${r + 2} ${y - 2}` : `L${r} ${y - 13}`
                    return {s: `M${a} ${t} V${y} H${l} ${end} M${r} ${y} H${b} V${t}`,
                            f: `M${l - 2.5} ${y} a2.5 2.5 0 1 0 5 0 a2.5 2.5 0 1 0 -5 0 Z`}
                }
                case "lautsprecher": { // Magnet mit Trichter nach oben
                    const a = cx(0), b = cx(1), m = (a + b) / 2
                    return {s: `M${a} ${t} V${y} H${m - 8} M${m + 8} ${y} H${b} V${t} M${m - 8} ${y - 4} H${m + 8} V${y + 4} H${m - 8} Z M${m - 6} ${y - 4} L${m - 12} ${y - 14} H${m + 12} L${m + 6} ${y - 4}`,
                            f: ""}
                }
                }
                return {s: "M0 0", f: ""}
            }
            ShapePath {
                strokeColor: "#D5E2EE"
                strokeWidth: 2
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                PathSvg { path: deskSymbol.paths.s }
            }
            ShapePath {
                strokeColor: "transparent"
                fillColor: "#D5E2EE"
                PathSvg { path: deskSymbol.paths.f || "M0 0" }
            }
            Text { // Pluszeichen am langen Strich (Polaritaet nie nur ueber die Strichlaenge)
                visible: deskSymbol.paths.plus !== undefined
                x: (deskSymbol.paths.plus || 0) - width / 2
                y: deskSymbol.y0 - 12 - height / 2
                text: "+"
                color: "#D5E2EE"
                font.pixelSize: 12
                font.bold: true
            }
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
        x: desk.centerOf("schalter") - width + 23 // Platte mittig, Beschriftung links davon
        y: 12
        width: 70
        height: 108
        readonly property bool on: desk.controller.deskSwitch
        Rectangle { // Metallplatte
            id: plate
            anchors.right: parent.right
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
            anchors.right: plate.left; anchors.rightMargin: 3; y: 4
            text: qsTr("ein"); color: parent.on ? "white" : "#8FA6BC"
            font.pixelSize: Math.round((11) * Theme.fontScale); font.bold: parent.on
        }
        Text {
            anchors.right: plate.left; anchors.rightMargin: 3; y: plate.height - height - 4
            text: qsTr("aus"); color: parent.on ? "#8FA6BC" : "white"
            font.pixelSize: Math.round((11) * Theme.fontScale); font.bold: !parent.on
        }
        Label {
            anchors.horizontalCenter: plate.horizontalCenter
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
