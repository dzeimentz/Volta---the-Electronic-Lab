// Gegenstaendliche Darstellung eines Bauteils (docs/ui-spec.md: Bauteile sehen aus wie Bauteile).
// Ausrichtung: links liegt der erste Anschluss des Katalogs (Anode, Plus, Basis, a) - ein umgedrehtes Bauteil
// erscheint damit um 180 Grad gedreht. Zustand ueber level (Helligkeit/Drehzahl/Lautstaerke), closed, destroyed.
// Farbe ist nie alleiniger Informationstraeger: Kathodenring, Plus-/Minus-Zeichen, Beschriftung, Form.
import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property string type
    property real value: 0
    property real level: 0
    property bool closed: false
    property bool destroyed: false
    property bool overloaded: false
    property bool selected: false

    implicitWidth: 72
    implicitHeight: 34

    readonly property bool isLed: type.startsWith("led")
    readonly property color ledColor: type === "led_gruen" ? "#22B14C" : type === "led_gelb" ? "#F2C318"
                                    : type === "led_blau" ? "#2C6BE0" : "#E8221B"

    // Farbringe eines Widerstands (4 Ringe: Ziffer, Ziffer, Multiplikator, Toleranz Gold)
    function bandColors(v) {
        const colors = ["#000000", "#8B4513", "#E8221B", "#FF8C00", "#F2D200", "#2E9A44", "#2C62C9", "#8A2BE2",
                        "#808080", "#FFFFFF"]
        if (!(v > 0)) return [colors[0], colors[0], colors[0], "#D4AF37"]
        let exp = Math.floor(Math.log10(v)) - 1
        let digits = Math.round(v / Math.pow(10, exp))
        if (digits >= 100) { digits = Math.round(digits / 10); exp += 1 }
        const d1 = Math.floor(digits / 10), d2 = digits % 10
        const mult = Math.max(0, Math.min(9, exp))
        return [colors[d1], colors[d2], colors[mult], "#D4AF37"]
    }

    // Ueberlast: pulsierender Glutrand (zusaetzlich zur Farbe: dicker Rand)
    Rectangle {
        anchors.centerIn: parent
        width: parent.width + 14
        height: parent.height + 14
        radius: height / 2
        color: "transparent"
        border.color: Theme.signalOrange
        border.width: 5
        visible: root.overloaded && !root.destroyed
        SequentialAnimation on opacity {
            running: root.overloaded && !root.destroyed
            loops: Animation.Infinite
            NumberAnimation { from: 0.3; to: 1; duration: 350 }
            NumberAnimation { from: 1; to: 0.3; duration: 350 }
        }
    }

    // Auswahl
    Rectangle {
        anchors.centerIn: parent
        width: parent.width + 8
        height: parent.height + 8
        radius: 8
        color: "transparent"
        border.color: Theme.accent
        border.width: 3
        visible: root.selected
    }

    Loader {
        anchors.fill: parent
        sourceComponent: {
            switch (root.type) {
            case "widerstand": return resistor
            case "gluehlampe": return lamp
            case "motor": return motor
            case "summer": return buzzer
            case "taster": return pushButton
            case "reedkontakt": return reed
            case "wechselschalter": return changeover
            case "kondensator": return capacitor
            case "elko": return electrolytic
            case "diode_si": case "diode_ge": return diode
            case "transistor_npn": return transistor
            }
            return root.isLed ? led : generic
        }
    }

    // Zerstoert: grau ueberdeckt, Rauch steigt auf, Beschriftung
    Rectangle {
        anchors.fill: parent
        radius: 6
        color: Theme.destroyed
        opacity: 0.75
        visible: root.destroyed
    }
    Repeater {
        model: root.destroyed ? 3 : 0
        Rectangle {
            id: puff
            required property int index
            width: 12 + index * 4; height: width; radius: width / 2
            color: "#8C8C8C"
            x: root.width / 2 - width / 2 + (index - 1) * 8
            y: -height
            SequentialAnimation on y {
                loops: Animation.Infinite
                PauseAnimation { duration: puff.index * 400 }
                NumberAnimation { from: -puff.height / 2; to: -40; duration: 1600 }
            }
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                PauseAnimation { duration: puff.index * 400 }
                NumberAnimation { from: 0.8; to: 0; duration: 1600 }
            }
        }
    }

    // ---------------------------------------------------------------- Bauteile
    Component {
        id: generic
        Rectangle { radius: 6; color: "#E9D7B0"; border.color: Theme.navy }
    }

    Component {
        id: resistor
        Item {
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.85; height: parent.height * 0.62
                radius: height / 2
                color: "#E2C9A0"
                border.color: "#7A6040"
                Row {
                    anchors.centerIn: parent
                    spacing: 6
                    Repeater {
                        model: root.bandColors(root.value)
                        Rectangle {
                            required property var modelData
                            required property int index
                            width: 5
                            height: parent.parent.height
                            color: modelData
                            // Abstand vor dem Toleranzring
                            anchors.verticalCenter: parent.verticalCenter
                            opacity: 1
                        }
                    }
                }
            }
        }
    }

    Component {
        id: led
        Item {
            // Leuchthof: Groesse und Deckkraft nach Strom (level = I / 20 mA)
            Rectangle {
                anchors.centerIn: kuppel
                width: kuppel.width * (1.4 + Math.min(1.5, root.level))
                height: width
                radius: width / 2
                color: root.ledColor
                opacity: root.destroyed ? 0 : Math.min(0.6, root.level * 0.5)
            }
            Rectangle {
                id: kuppel
                anchors.centerIn: parent
                width: parent.height * 0.95; height: width; radius: width / 2
                color: root.level > 0.05 && !root.destroyed ? Qt.lighter(root.ledColor, 1.0 + Math.min(0.8, root.level * 0.6))
                                                          : Qt.darker(root.ledColor, 1.3)
                border.color: Qt.darker(root.ledColor, 2)
                border.width: 2
            }
            // Kathode (rechts): abgeflachte Seite und Strich, unabhaengig von der Farbe erkennbar
            Rectangle {
                x: kuppel.x + kuppel.width - 5; y: kuppel.y + 3
                width: 4; height: kuppel.height - 6
                color: Theme.navy
            }
            Text { x: kuppel.x - 12; anchors.verticalCenter: parent.verticalCenter; text: "+"; font.pixelSize: 14; font.bold: true; color: Theme.navy }
        }
    }

    Component {
        id: diode
        Item {
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.7; height: parent.height * 0.5
                radius: 4
                color: root.type === "diode_ge" ? "#C9B07A" : "#202020"
                // Kathodenring rechts
                Rectangle { anchors.right: parent.right; anchors.rightMargin: 4; width: 6; height: parent.height; color: "#D9D9D9" }
            }
        }
    }

    Component {
        id: lamp
        Item {
            Rectangle { // Glimmen nach Leistung (level = P / P_nenn)
                anchors.centerIn: bulb
                width: bulb.width * (1.3 + Math.min(1.2, root.level))
                height: width; radius: width / 2
                color: "#FFD966"
                opacity: root.destroyed ? 0 : Math.min(0.7, root.level * 0.6)
            }
            Rectangle {
                id: bulb
                anchors.centerIn: parent
                width: parent.height * 1.1; height: width; radius: width / 2
                color: Qt.rgba(1, 0.95, 0.75 - Math.min(0.5, root.level * 0.4), 0.9)
                border.color: "#9C8F5A"
                border.width: 2
                // Glühfaden: dunkel, gluehend orange, unterbrochen wenn durchgebrannt
                Shape {
                    anchors.fill: parent
                    ShapePath {
                        strokeColor: root.destroyed ? "#555555" : (root.level > 0.05 ? "#FF7A1A" : "#6B5B3A")
                        strokeWidth: 2
                        fillColor: "transparent"
                        startX: bulb.width * 0.25; startY: bulb.height * 0.6
                        PathLine { x: bulb.width * 0.4; y: bulb.height * 0.35 }
                        PathLine { x: bulb.width * 0.5; y: root.destroyed ? bulb.height * 0.45 : bulb.height * 0.6 }
                        PathMove { x: bulb.width * 0.55; y: bulb.height * 0.6 }
                        PathLine { x: bulb.width * 0.6; y: bulb.height * 0.35 }
                        PathLine { x: bulb.width * 0.75; y: bulb.height * 0.6 }
                    }
                }
            }
        }
    }

    Component {
        id: motor
        Item {
            Rectangle {
                id: housing
                anchors.centerIn: parent
                width: parent.height * 1.3; height: width; radius: width / 2
                color: "#B8BEC6"
                border.color: "#5A6470"
                border.width: 2
                Text { anchors.centerIn: parent; text: "M"; font.pixelSize: 14; font.bold: true; color: Theme.navy }
            }
            // Propeller: dreht mit der simulierten Drehzahl (level = omega / Leerlauf), Richtung nach Vorzeichen
            Item {
                id: prop
                anchors.centerIn: housing
                width: housing.width * 2.2; height: width
                Rectangle { anchors.centerIn: parent; width: parent.width; height: 8; radius: 4; color: "#2E7DBE"; opacity: 0.85 }
                Rectangle { anchors.centerIn: parent; width: 8; height: parent.height; radius: 4; color: "#2E7DBE"; opacity: 0.85 }
                FrameAnimation {
                    running: Math.abs(root.level) > 0.005 && !root.destroyed
                    onTriggered: prop.rotation = (prop.rotation + root.level * 1080 * frameTime) % 360
                }
            }
        }
    }

    Component {
        id: buzzer
        Item {
            Rectangle {
                id: can
                anchors.centerIn: parent
                width: parent.height * 1.1; height: width; radius: width / 2
                color: "#1E1E1E"
                Rectangle { anchors.centerIn: parent; width: 6; height: 6; radius: 3; color: "#555555" }
                Text { x: -12; anchors.verticalCenter: parent.verticalCenter; text: "+"; font.pixelSize: 14; font.bold: true; color: Theme.navy }
            }
            // Schallwellen: sichtbar, auch wenn der Ton stumm geschaltet ist (Klassenmodus)
            Repeater {
                model: root.level > 0.02 && !root.destroyed ? 2 : 0
                Rectangle {
                    id: wave
                    required property int index
                    anchors.centerIn: can
                    width: can.width; height: width; radius: width / 2
                    color: "transparent"
                    border.color: Theme.signalOrange
                    border.width: 2
                    SequentialAnimation on scale {
                        loops: Animation.Infinite
                        PauseAnimation { duration: wave.index * 250 }
                        NumberAnimation { from: 1; to: 1.9; duration: 500 }
                    }
                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        PauseAnimation { duration: wave.index * 250 }
                        NumberAnimation { from: root.level; to: 0; duration: 500 }
                    }
                }
            }
        }
    }

    Component {
        id: pushButton
        Item {
            Rectangle {
                anchors.centerIn: parent
                width: parent.height * 1.2; height: width; radius: 6
                color: "#2B2B2B"
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width * (root.closed ? 0.5 : 0.62); height: width; radius: width / 2
                    color: root.closed ? "#B23A00" : "#E8221B"
                    border.color: "#111111"
                }
            }
        }
    }

    Component {
        id: reed
        Item {
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.8; height: parent.height * 0.45; radius: height / 2
                color: "#DDEFF5"; border.color: "#7FA9B8"
                // Kontaktzungen: geschlossen beruehren sie sich
                Rectangle { x: 4; anchors.verticalCenter: parent.verticalCenter; width: parent.width / 2 - (root.closed ? 2 : 8); height: 3; color: "#555555" }
                Rectangle { x: parent.width / 2 + (root.closed ? -2 : 4); anchors.verticalCenter: parent.verticalCenter; width: parent.width / 2 - 4; height: 3; color: "#555555"; rotation: root.closed ? 0 : -8 }
            }
            Text {
                visible: root.closed
                anchors.bottom: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                text: "🧲"; font.pixelSize: 14
            }
        }
    }

    Component {
        id: changeover
        Item {
            Rectangle {
                anchors.fill: parent; radius: 6
                color: "#E9D7B0"; border.color: Theme.navy
                Text { x: 6; anchors.verticalCenter: parent.verticalCenter; text: "A"; font.pixelSize: 11; color: Theme.navy }
                Text { anchors.right: parent.right; anchors.rightMargin: 6; anchors.verticalCenter: parent.verticalCenter; text: "B"; font.pixelSize: 11; color: Theme.navy }
                Rectangle { // Hebel zeigt zur geschlossenen Seite
                    anchors.centerIn: parent
                    width: parent.width * 0.4; height: 6; radius: 3
                    color: Theme.navy
                    rotation: root.closed ? 30 : -30
                    transformOrigin: Item.Center
                }
            }
        }
    }

    Component {
        id: capacitor
        Rectangle {
            anchors.centerIn: parent
            width: parent.height * 0.9; height: parent.height * 0.9; radius: 4
            color: "#E2A33B"; border.color: "#8A5A10"
        }
    }

    Component {
        id: electrolytic
        Item {
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.8; height: parent.height * 0.75; radius: 6
                color: "#2C62C9"
                // Minus-Streifen rechts (zweiter Anschluss), mit Zeichen
                Rectangle { anchors.right: parent.right; width: 10; height: parent.height; radius: 4; color: "#C9D6F0"
                    Text { anchors.centerIn: parent; text: "−"; font.pixelSize: 12; font.bold: true; color: "#2C62C9" } }
                Text { x: 4; anchors.verticalCenter: parent.verticalCenter; text: "+"; font.pixelSize: 12; font.bold: true; color: "white" }
            }
        }
    }

    Component {
        id: transistor
        Rectangle {
            anchors.fill: parent; radius: 4
            color: "#3E7C3A"; border.color: "#1F4A1C"
            Row {
                anchors.centerIn: parent
                spacing: parent.width / 5
                Repeater {
                    model: ["B", "C", "E"]
                    Text { required property string modelData; text: modelData; font.pixelSize: 11; font.bold: true; color: "white" }
                }
            }
        }
    }
}
