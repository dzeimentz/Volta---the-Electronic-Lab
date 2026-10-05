// Dreifach-Messgeraet (docs/ui-spec.md, Bedienpult): Anzeige (digital: 3 Stellen + Einheit, 3 Hz; analog: Zeiger mit
// Traegheit und leichtem Ueberschwingen in allen Messarten; Ohm-Skala rueckwaerts und nichtlinear wie beim
// Zeiger-Ohmmeter), Drehschalter V / A / Ohm, Umschalter digital/analog.
// Die Buchsen COM, V/Ohm, A sind Kontaktfelder der Arbeitsflaeche (Lab.qml) und sitzen unten in diesem Rahmen.
// Messart und Anzeigeart sind Bedienhandlungen; »Sicherung tauschen« ist ein Befehl.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Rectangle {
    id: meter
    required property var controller
    property var state: ({})
    readonly property int mode: controller.meterMode // 0 V, 1 A, 2 Ohm
    readonly property bool analog: controller.meterAnalog

    radius: 12
    color: "#2B3440"
    border.color: "#151A20"
    border.width: 2

    function refresh() { meter.state = meter.controller.meterState() }
    // Digitalanzeige mit 3 Hz (docs/ui-spec.md); der Zeiger folgt dem Simulationstakt der Oberflaeche
    Timer { interval: 333; repeat: true; running: meter.visible; triggeredOnStart: true; onTriggered: meter.refresh() }
    Connections {
        target: meter.controller
        enabled: meter.analog
        function onStateChanged() { needle.level = meter.controller.meterState().level || 0 }
    }

    Label {
        id: title
        anchors.horizontalCenter: parent.horizontalCenter
        y: 8
        text: qsTr("Messgerät")
        color: "white"
        font.pixelSize: Math.round((14) * Theme.fontScale)
        font.bold: true
    }

    // ---------------------------------------------------------------- Anzeige
    Rectangle {
        id: display
        objectName: "meterDisplay"
        x: 12
        y: title.y + title.height + 8
        width: parent.width - 24
        height: 92
        radius: 6
        color: meter.analog ? "#F4F1E6" : "#B9CBA4"
        border.color: "#10151B"
        border.width: 3

        // digital
        Column {
            visible: !meter.analog
            anchors.centerIn: parent
            spacing: 2
            Label {
                id: digits
                objectName: "meterDigits"
                anchors.horizontalCenter: parent.horizontalCenter
                text: meter.state.fuseBlown ? "- - -"
                    : meter.state.foreignVoltage ? "U !"
                    : meter.state.overrange ? "OL"
                    : (meter.state.text || "")
                font.family: "Consolas"
                font.pixelSize: Math.round((34) * Theme.fontScale)
                font.bold: true
                color: "#18221A"
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: meter.state.fuseBlown || meter.state.foreignVoltage || meter.state.overrange ? "" : (meter.state.unit || "")
                font.pixelSize: Math.round((16) * Theme.fontScale)
                font.bold: true
                color: "#18221A"
            }
        }

        // analog: Skala je Messart, Zeiger mit Feder (Ueberschwingen)
        Item {
            id: dial
            visible: meter.analog
            anchors.fill: parent
            readonly property real cx: width / 2
            readonly property real cy: height - 10
            readonly property real r: 54
            readonly property real range: meter.state.range || 0
            // Skalenpunkte als Anteil 0..1 des Ausschlags; Ohm: p = Rm / (Rm + R) fuer R = oo, 10, 3, 1, 1/3, 1/10, 0 * Rm
            readonly property var ticks: meter.mode === 2 ? [0, 1 / 11, 0.25, 0.5, 0.75, 10 / 11, 1]
                                                          : [0, .1, .2, .3, .4, .5, .6, .7, .8, .9, 1]
            readonly property var majors: meter.mode === 2 ? [0, 0.5, 1] : [0, 0.5, 1]
            function ohmText(v) {
                if (v >= 1e6) return (v / 1e6).toLocaleString(Qt.locale(), "f", 0) + "M"
                if (v >= 1e3) return (v / 1e3).toLocaleString(Qt.locale(), "f", 0) + "k"
                return v.toLocaleString(Qt.locale(), "f", 0)
            }
            function num(v) { return v.toLocaleString(Qt.locale(), "f", v < 10 && v % 1 !== 0 ? 1 : 0) }
            // Beschriftung an den Hauptpunkten links, Mitte, rechts
            function majorText(i) {
                if (meter.mode === 2) return i === 0 ? "∞" : (i === 1 ? ohmText(range) : "0")
                const v = range * i / 2
                return meter.mode === 1 && range < 1 ? num(v * 1000) : num(v)
            }
            readonly property string unit: meter.mode === 0 ? "V" : meter.mode === 1 ? (range < 1 ? "mA" : "A") : "Ω"

            Repeater {
                model: dial.ticks
                Rectangle {
                    required property real modelData
                    readonly property real deg: -50 + modelData * 100
                    readonly property bool major: dial.majors.indexOf(modelData) >= 0
                    width: 2
                    height: major ? 12 : 7
                    color: "#222222"
                    x: dial.cx + Math.sin(deg * Math.PI / 180) * dial.r - 1
                    y: dial.cy - Math.cos(deg * Math.PI / 180) * dial.r - height
                    rotation: deg
                    transformOrigin: Item.Bottom
                }
            }
            Repeater {
                model: 3
                Label {
                    required property int index
                    readonly property real a: (-50 + index * 50) * Math.PI / 180
                    text: dial.majorText(index)
                    font.pixelSize: Math.round((11) * Theme.fontScale)
                    color: "#222222"
                    x: dial.cx + Math.sin(a) * (dial.r + 12) - width / 2
                    y: dial.cy - Math.cos(a) * (dial.r + 12) - height
                }
            }
            Label { objectName: "meterUnit"; x: 6; y: 4; text: dial.unit; font.pixelSize: Math.round((13) * Theme.fontScale); font.bold: true; color: "#222222" }
            Rectangle {
                id: needle
                objectName: "meterNeedle"
                property real level: 0
                width: 3
                height: dial.r + 4
                radius: 1.5
                color: "#C62828"
                x: dial.cx - 1.5
                y: dial.cy - height
                transformOrigin: Item.Bottom
                // 0 ... 1 -> -50 ... +50 Grad; Anschlag knapp darueber/darunter (Gegenrichtung: links am Anschlag)
                rotation: -50 + Math.max(-0.04, Math.min(1.06, level)) * 100
                Behavior on rotation { enabled: !Theme.reduceMotion; SpringAnimation { spring: 4; damping: 0.22; epsilon: 0.05 } }
            }
            Rectangle { width: 10; height: 10; radius: 5; color: "#333333"; x: dial.cx - 5; y: dial.cy - 5 }
        }
        Accessible.role: Accessible.StaticText
        Accessible.name: meter.state.fuseBlown ? qsTr("Sicherung durchgebrannt")
                       : meter.state.overrange ? qsTr("Messbereich überschritten")
                       : (meter.state.text || "") + " " + (meter.state.unit || "")
    }

    // Hinweise unter der Anzeige (Text, nicht nur Farbe)
    Label {
        id: hint
        x: 12
        y: display.y + display.height + 4
        width: parent.width - 24
        visible: text !== ""
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        text: meter.state.fuseBlown ? qsTr("Sicherung durchgebrannt!")
            : meter.state.foreignVoltage ? qsTr("Spannung abschalten!")
            : (meter.analog && meter.state.overrange && meter.mode !== 2) ? qsTr("Zu viel für den Zeiger")
            : ""
        color: Theme.solderAmber
        font.pixelSize: Math.round((12) * Theme.fontScale)
        font.bold: true
    }

    // ---------------------------------------------------------------- Drehschalter V / A / Ohm
    Item {
        id: selector
        x: 0
        y: display.y + display.height + 30
        width: parent.width
        height: 104
        readonly property var labels: ["V", "A", "Ω"]
        readonly property var angles: [-45, 0, 45]

        Rectangle {
            id: knob
            objectName: "meterKnob"
            width: 52; height: 52; radius: 26
            x: parent.width / 2 - 26
            y: parent.height - 58
            color: "#11161C"
            border.color: "#5C6B7A"
            border.width: 2
            rotation: selector.angles[meter.mode]
            Behavior on rotation { NumberAnimation { duration: 120 } }
            Rectangle { anchors.horizontalCenter: parent.horizontalCenter; y: 4; width: 5; height: 18; radius: 2; color: "white" }
            TapHandler { onTapped: meter.controller.meterMode = (meter.mode + 1) % 3 }
            Accessible.role: Accessible.Dial
            Accessible.name: qsTr("Messart")
        }
        Repeater {
            model: 3
            Item {
                required property int index
                objectName: "meterMode_" + index
                readonly property real a: selector.angles[index] * Math.PI / 180
                width: Theme.control; height: Theme.control
                x: selector.width / 2 + Math.sin(a) * 52 - width / 2
                y: knob.y + 26 - Math.cos(a) * 52 - height / 2
                Rectangle {
                    anchors.centerIn: parent
                    width: 34; height: 34; radius: 17
                    color: meter.mode === index ? Theme.signalOrange : "transparent"
                    border.color: meter.mode === index ? Theme.solderAmber : "#7F93A8"
                    border.width: 2
                    Label {
                        anchors.centerIn: parent
                        text: selector.labels[index]
                        color: "white"
                        font.pixelSize: Math.round((16) * Theme.fontScale)
                        font.bold: true
                    }
                }
                TapHandler { onTapped: meter.controller.meterMode = index }
                Accessible.role: Accessible.RadioButton
                Accessible.name: [qsTr("Spannung messen"), qsTr("Strom messen"), qsTr("Widerstand messen")][index]
            }
        }
    }

    // ---------------------------------------------------------------- digital / analog bzw. Sicherung tauschen
    Row {
        visible: !meter.state.fuseBlown
        anchors.horizontalCenter: parent.horizontalCenter
        y: selector.y + selector.height + 2
        spacing: 2
        Label { anchors.verticalCenter: parent.verticalCenter; text: qsTr("digital"); color: "white"; font.pixelSize: Math.round((12) * Theme.fontScale) }
        Switch {
            objectName: "meterAnalog"
            checked: meter.controller.meterAnalog
            onToggled: meter.controller.meterAnalog = checked
            Accessible.name: qsTr("Zeigeranzeige")
        }
        Label {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("analog")
            color: "white"
            font.pixelSize: Math.round((12) * Theme.fontScale)
        }
    }
    Button {
        objectName: "meterFuse"
        visible: meter.state.fuseBlown === true
        anchors.horizontalCenter: parent.horizontalCenter
        y: selector.y + selector.height + 2
        height: Theme.control
        text: qsTr("Sicherung tauschen")
        onClicked: meter.controller.replaceMeterFuse()
    }
}
