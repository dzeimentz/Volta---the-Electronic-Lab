// Gegenstaendliche Darstellung eines Bauteils (docs/ui-spec.md: Bauteile sehen aus wie Bauteile).
// Ausrichtung: links liegt der erste Anschluss des Katalogs (Anode, Plus, Basis, a) - ein umgedrehtes Bauteil
// erscheint damit um 180 Grad gedreht. Zustand ueber level (Helligkeit/Drehzahl/Lautstaerke), closed, destroyed.
// Farbe ist nie alleiniger Informationstraeger: Kathodenring, Plus-/Minus-Zeichen, Beschriftung, Form.
import QtQuick
import QtQuick.Shapes

Item {
    id: root
    // Pluszeichen gepolter Bauteile (LED, Summer, Elko, Piezo): rot, kraeftig, mit heller Kontur und unterhalb der
    // Mittellinie, damit es neben dem eingesteckten Bein sichtbar bleibt
    component PlusMark: Item {
        width: 16; height: 16
        Rectangle { anchors.centerIn: parent; width: 16; height: 5; radius: 1; color: "#D32F2F"; border.color: "white"; border.width: 1 }
        Rectangle { anchors.centerIn: parent; width: 5; height: 16; radius: 1; color: "#D32F2F"; border.color: "white"; border.width: 1 }
        Rectangle { anchors.centerIn: parent; width: 3; height: 3; color: "#D32F2F" } // Kreuzung ohne Kontur
    }
    property string type
    property real value: 0
    property real level: 0
    property bool closed: false
    property real position: 0.5 // Trimmer: Stellung des Schleifers 0..1
    property bool destroyed: false
    property bool overloaded: false
    property bool selected: false
    property var feet: [] // Fusspunkte relativ zum Koerper (quadratische Platine)
    property int tag: 0 // Messstelle: Nummer 1 oder 2

    implicitWidth: 72
    implicitHeight: 34

    // Grosse Koerper zeichnerisch kleiner (Beine reichen weiter bis zu den Steckpunkten); Symbole unberuehrt
    readonly property real sizeFactor: isLed || type === "ir_led" || type.startsWith("trimmer") || type === "drehkondensator"
        || ["taster", "wechselschalter", "reedkontakt", "widerstand", "kondensator", "elko", "motor"].indexOf(type) >= 0
        ? 0.85 : 1.0
    scale: sizeFactor

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
            case "diode_si": case "diode_ge": case "zdiode_4v7": case "zdiode_5v6": return diode
            case "transistor_npn": case "transistor_pnp": return transistor
            case "spule_100mh": case "spule_1h": case "spule_220uh": case "spule_1mh": case "spule_10mh": return coil
            case "drehkondensator": return variableCap
            case "trimmer_10k": case "trimmer_100k": return trimmer
            case "ir_led": return irLed
            case "fototransistor": return phototransistor
            case "optokoppler": return optocoupler
            case "spannungsregler_5v": return regulator
            case "fotowiderstand": return photo
            case "heissleiter": return thermistor
            case "elektromagnet": return magnet
            case "relais": return relay
            case "piezo": return piezo
            case "feuchtesonde": return probe
            case "messstelle_v": case "messstelle_a": case "messstelle_ohm": return measurePoint
            case "tastkopf": return scopeProbe
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
            PlusMark { x: kuppel.x - 19; y: parent.height / 2 + 1 }
        }
    }

    Component {
        id: diode
        Item {
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.7; height: parent.height * 0.5
                radius: 4
                readonly property bool zener: root.type.startsWith("zdiode")
                // Z-Diode: Glasgehaeuse (orange) mit schwarzem Ring und aufgedruckter Spannung (Farbe nie allein)
                color: zener ? "#E39A4C" : root.type === "diode_ge" ? "#C9B07A" : "#202020"
                // Kathodenring rechts
                Rectangle { anchors.right: parent.right; anchors.rightMargin: 4; width: 6; height: parent.height
                            color: parent.zener ? "#202020" : "#D9D9D9" }
                Text {
                    visible: parent.zener
                    anchors.centerIn: parent; anchors.horizontalCenterOffset: -4
                    text: root.type === "zdiode_4v7" ? "4V7" : "5V6"
                    color: "#202020"
                    font.pixelSize: Math.round(10 * Theme.fontScale); font.bold: true
                }
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
                Text { anchors.centerIn: parent; text: "M"; font.pixelSize: Math.round((14) * Theme.fontScale); font.bold: true; color: Theme.navy }
            }
            // Propeller: dreht mit der simulierten Drehzahl (level = omega / Leerlauf), Richtung nach Vorzeichen
            Item {
                id: prop
                anchors.centerIn: housing
                width: housing.width * 2.2; height: width
                rotation: 45 // Grundstellung als X (Fluegel schraeg, im Bauteilfach flacher)
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
                PlusMark { x: -19; y: parent.height / 2 + 1 }
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
                text: "🧲"; font.pixelSize: Math.round((14) * Theme.fontScale)
            }
        }
    }

    Component {
        id: changeover
        Item {
            Rectangle {
                anchors.fill: parent; radius: 6
                color: "#E9D7B0"; border.color: Theme.navy
                Text { x: 6; anchors.verticalCenter: parent.verticalCenter; text: "A"; font.pixelSize: Math.round((11) * Theme.fontScale); color: Theme.navy }
                Text { anchors.right: parent.right; anchors.rightMargin: 6; anchors.verticalCenter: parent.verticalCenter; text: "B"; font.pixelSize: Math.round((11) * Theme.fontScale); color: Theme.navy }
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
            // ab Volta 2: Kondensatorcode wie auf dem echten Bauteil (104 = 10 · 10^4 pF = 100 nF)
            Text {
                anchors.centerIn: parent
                visible: Texts.band !== "" && Texts.band !== "volta1" && root.value > 0
                text: Texts.capacitorCode(root.value)
                color: "#3A2A08"
                font.pixelSize: Math.round(10 * Theme.fontScale); font.bold: true
            }
        }
    }

    // IR-LED (ab Volta 2): dunkle Kuppel, leuchtet nicht sichtbar; im Betrieb gestrichelte Strahlen und »IR«
    Component {
        id: irLed
        Item {
            Repeater { // Strahlen nach Strom (level = I / 20 mA), gestrichelt: unsichtbares Licht
                model: root.level > 0.05 && !root.destroyed ? [-35, 0, 35] : []
                Row {
                    required property int modelData
                    x: irDome.x + irDome.width / 2
                    y: irDome.y + irDome.height / 2 - 1
                    spacing: 3
                    transformOrigin: Item.Left
                    rotation: -90 + modelData
                    Item { width: irDome.width / 2 + 1; height: 1 } // Strahlen beginnen am Kuppelrand
                    Repeater {
                        model: 3
                        Rectangle { width: 6; height: 3; radius: 1; color: "#8E5CE6"; opacity: Math.min(1, 0.5 + root.level * 0.4) }
                    }
                }
            }
            Rectangle {
                id: irDome
                anchors.centerIn: parent
                width: parent.height * 0.95; height: width; radius: width / 2
                color: "#4A4458"; border.color: "#1E1A28"; border.width: 2
                Text { anchors.centerIn: parent; text: "IR"; color: "#E6E0F2"; font.pixelSize: Math.round(10 * Theme.fontScale); font.bold: true }
            }
            Rectangle { // Kathode rechts: abgeflachte Seite und Strich
                x: irDome.x + irDome.width - 5; y: irDome.y + 3
                width: 4; height: irDome.height - 6
                color: Theme.navy
            }
            PlusMark { x: irDome.x - 19; y: parent.height / 2 + 1 }
        }
    }

    // Fototransistor (ab Volta 2): klare Kuppel mit Chip; C (Kollektor, erster Anschluss) links, E rechts aufgedruckt
    Component {
        id: phototransistor
        Item {
            Rectangle {
                id: ptDome
                anchors.centerIn: parent
                width: parent.height * 0.95; height: width; radius: width / 2
                color: "#DCE8F2"; border.color: "#6F8396"; border.width: 2
                Rectangle { anchors.centerIn: parent; width: 8; height: 8; color: "#2E3B4A" }
                Rectangle { // leuchtet auf, wenn Strom fliesst (level = I / 1 mA)
                    anchors.fill: parent; radius: width / 2
                    color: "#FFE58A"
                    opacity: root.destroyed ? 0 : Math.min(0.5, root.level * 0.25)
                }
            }
            Text { anchors.right: ptDome.left; anchors.rightMargin: 2; anchors.verticalCenter: parent.verticalCenter; text: "C"; font.pixelSize: Math.round(11 * Theme.fontScale); font.bold: true; color: Theme.navy }
            Text { anchors.left: ptDome.right; anchors.leftMargin: 2; anchors.verticalCenter: parent.verticalCenter; text: "E"; font.pixelSize: Math.round(11 * Theme.fontScale); font.bold: true; color: Theme.navy }
        }
    }

    // Optokoppler (ab Volta 2) auf 2-x-2-Platine: schwarzes Gehaeuse mit Punkt bei Anschluss 1 (Anode);
    // Fussbezeichnung A, K (IR-LED) und C, E (Fototransistor)
    Component {
        id: optocoupler
        Rectangle {
            id: okPcb
            anchors.fill: parent; radius: 6
            color: "#2F6B3A"; border.color: "#173D1E"; border.width: 2
            readonly property var f: root.feet.length === 4 ? root.feet
                                   : [Qt.point(16, 14), Qt.point(16, height - 14),
                                      Qt.point(width - 16, 14), Qt.point(width - 16, height - 14)]
            Rectangle {
                anchors.centerIn: parent
                width: 28; height: 24; radius: 3
                color: "#1E1E1E"; border.color: "#000000"
                Rectangle { x: 4; y: 4; width: 5; height: 5; radius: 2.5; color: "#BDBDBD" } // Punkt: Anschluss 1
                Text { anchors.centerIn: parent; anchors.verticalCenterOffset: 3; text: "OK"; color: "#E0E0E0"; font.pixelSize: Math.round(9 * Theme.fontScale); font.bold: true }
            }
            Repeater {
                model: ["A", "K", "C", "E"]
                Text {
                    required property int index
                    required property string modelData
                    text: modelData
                    font.pixelSize: Math.round((11) * Theme.fontScale); font.bold: true; color: "white"
                    x: okPcb.f[index].x + (okPcb.f[index].x < okPcb.width / 2 ? 7 : -7 - width)
                    y: okPcb.f[index].y + (okPcb.f[index].y < okPcb.height / 2 ? 4 : -4 - height)
                }
            }
        }
    }

    // Spannungsregler 5 V (ab Volta 2) auf Traegerplatine: TO-92-Gehaeuse mit Aufdruck, Anschluesse IN, GND, OUT;
    // wird bei Ueberlast bzw. Uebertemperatur ueber den Glutrand angezeigt
    Component {
        id: regulator
        Rectangle {
            anchors.fill: parent; radius: 6
            color: "#E9D7B0"; border.color: Theme.navy
            Rectangle {
                anchors.centerIn: parent
                width: 34; height: parent.height * 0.82; radius: 4
                color: "#1E1E1E"; border.color: "#000000"
                Rectangle { anchors.top: parent.top; width: parent.width; height: parent.height / 2; radius: 10; color: "#1E1E1E" }
                Text { anchors.centerIn: parent; text: "5V"; color: "#E0E0E0"; font.pixelSize: Math.round(11 * Theme.fontScale); font.bold: true }
            }
            Text { x: 5; anchors.verticalCenter: parent.verticalCenter; text: "IN"; font.pixelSize: Math.round(9 * Theme.fontScale); font.bold: true; color: Theme.navy }
            Text { anchors.right: parent.right; anchors.rightMargin: 5; anchors.verticalCenter: parent.verticalCenter; text: "OUT"; font.pixelSize: Math.round(9 * Theme.fontScale); font.bold: true; color: Theme.navy }
            Text { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: -1; text: "GND"; font.pixelSize: Math.round(8 * Theme.fontScale); font.bold: true; color: Theme.navy }
        }
    }

    // Trimmer (ab Volta 2) auf Traegerplatine: blaues Gehaeuse, Drehscheibe mit Schlitz zeigt die Stellung
    // (links Anschlag a, rechts Anschlag b); a, s, b aufgedruckt
    Component {
        id: trimmer
        Rectangle {
            anchors.fill: parent; radius: 6
            color: "#E9D7B0"; border.color: Theme.navy
            Text { x: 6; anchors.verticalCenter: parent.verticalCenter; text: "a"; font.pixelSize: Math.round(11 * Theme.fontScale); color: Theme.navy }
            Text { anchors.right: parent.right; anchors.rightMargin: 6; anchors.verticalCenter: parent.verticalCenter; text: "b"; font.pixelSize: Math.round(11 * Theme.fontScale); color: Theme.navy }
            Rectangle {
                anchors.centerIn: parent
                width: parent.height * 0.92; height: width; radius: 4
                color: "#2C62C9"; border.color: "#173A80"
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width * 0.72; height: width; radius: width / 2
                    color: "#F2F2F2"; border.color: "#7A7A7A"
                    Rectangle { // Schlitz: -135 Grad (a) bis +135 Grad (b)
                        anchors.centerIn: parent
                        width: parent.width * 0.85; height: 3; radius: 1.5
                        color: "#303030"
                        rotation: 90 + (root.position - 0.5) * 270
                    }
                }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom; anchors.bottomMargin: -1
                text: root.type === "trimmer_100k" ? "100k" : "10k"
                font.pixelSize: Math.round(8 * Theme.fontScale); font.bold: true; color: Theme.navy
            }
        }
    }

    // Drehkondensator (ab Volta 3): durchsichtiges Gehaeuse mit Plattenpaket, Drehknopf mit Strich zeigt die Stellung
    // (links 10 pF, rechts 500 pF); Bereich aufgedruckt
    Component {
        id: variableCap
        Rectangle {
            anchors.fill: parent; radius: 6
            color: "#DCE6EC"; border.color: Theme.navy
            Row { // Plattenpaket
                x: 5; anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                Repeater {
                    model: 5
                    Rectangle { width: 2; height: root.height * 0.6; color: "#9AA6B0" }
                }
            }
            Rectangle {
                anchors.centerIn: parent
                width: parent.height * 0.92; height: width; radius: width / 2
                color: "#F2F2F2"; border.color: "#5A5A5A"
                Rectangle { // Zeigerstrich: -135 Grad (10 pF) bis +135 Grad (500 pF)
                    anchors.centerIn: parent
                    width: 3; height: parent.height * 0.85; radius: 1.5
                    color: "transparent"
                    rotation: (root.position - 0.5) * 270
                    Rectangle { width: parent.width; height: parent.height / 2; radius: 1.5; color: "#303030" }
                }
            }
            Text {
                anchors.right: parent.right; anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                text: "pF"
                font.pixelSize: Math.round(9 * Theme.fontScale); font.bold: true; color: Theme.navy
            }
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
                    Text { anchors.centerIn: parent; text: "−"; font.pixelSize: Math.round((12) * Theme.fontScale); font.bold: true; color: "#2C62C9" } }
                PlusMark { x: 3; y: parent.height - height - 1 }
            }
        }
    }

    // Transistor auf quadratischer Platine (Eigenentwurf): Fuesse B | C oben, E | E unten; die beiden E-Fuesse
    // verbindet eine Leiterbahn. In der Mitte das Gehaeuse mit flacher Seite und das Schaltzeichen-Kuerzel.
    Component {
        id: transistor
        Rectangle {
            id: pcb
            anchors.fill: parent; radius: 6
            color: "#2F6B3A"; border.color: "#173D1E"; border.width: 2
            readonly property var f: root.feet.length === 4 ? root.feet
                                   : [Qt.point(16, 14), Qt.point(width - 16, 14),
                                      Qt.point(16, height - 14), Qt.point(width - 16, height - 14)]
            // Leiterbahnen: E <-> E sichtbar, B/C/E zum Gehaeuse
            Repeater {
                model: [[pcb.f[2], pcb.f[3]], [pcb.f[0], Qt.point(pcb.width / 2, pcb.height / 2)],
                        [pcb.f[1], Qt.point(pcb.width / 2, pcb.height / 2)],
                        [Qt.point((pcb.f[2].x + pcb.f[3].x) / 2, (pcb.f[2].y + pcb.f[3].y) / 2), Qt.point(pcb.width / 2, pcb.height / 2)]]
                Rectangle {
                    required property var modelData
                    readonly property real dx: modelData[1].x - modelData[0].x
                    readonly property real dy: modelData[1].y - modelData[0].y
                    x: modelData[0].x; y: modelData[0].y - height / 2
                    width: Math.sqrt(dx * dx + dy * dy); height: 4
                    transformOrigin: Item.Left
                    rotation: Math.atan2(dy, dx) * 180 / Math.PI
                    color: "#C9A24A"
                }
            }
            // Gehaeuse (halbrund mit flacher Seite)
            Rectangle {
                anchors.centerIn: parent
                width: 26; height: 22; radius: 11
                color: "#1E1E1E"; border.color: "#000000"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: parent.height / 2; color: "#1E1E1E" }
                // NPN: »T«, PNP: »P« (ab Volta 2; die Bauart steht auf dem Gehaeuse, nicht nur in der Farbe)
                Text { anchors.centerIn: parent; text: root.type === "transistor_pnp" ? "P" : "T"; color: "#E0E0E0"; font.pixelSize: Math.round((11) * Theme.fontScale); font.bold: true }
            }
            // Fussbezeichnung (Farbe nie allein: Buchstaben)
            Repeater {
                model: ["B", "C", "E", "E"]
                Text {
                    required property int index
                    required property string modelData
                    text: modelData
                    font.pixelSize: Math.round((11) * Theme.fontScale); font.bold: true; color: "white"
                    // neben dem Fuss, zur Platinenmitte hin (folgt der Drehung)
                    x: pcb.f[index].x + (pcb.f[index].x < pcb.width / 2 ? 7 : -7 - width)
                    y: pcb.f[index].y + (pcb.f[index].y < pcb.height / 2 ? 4 : -4 - height)
                }
            }
        }
    }

    // Spule (ab Volta 2): Spulenkoerper mit Kupferwicklung, 1 H mit Eisenkern (Kernenden grau); Wert aufgedruckt
    Component {
        id: coil
        Item {
            readonly property bool core: root.type === "spule_1h"
            Rectangle { // Kern bzw. Spulenkoerper ragt seitlich heraus
                anchors.centerIn: parent
                width: parent.width * 0.78; height: parent.height * 0.42; radius: 3
                color: parent.core ? "#7A7F86" : "#E8E2D0"; border.color: "#5A5A5A"
            }
            Rectangle {
                id: winding
                anchors.centerIn: parent
                width: parent.width * 0.7; height: parent.height * 0.8; radius: 5
                color: "#C8742F"; border.color: "#7A3E10"; border.width: 1
                Row { // Windungen
                    anchors.centerIn: parent
                    spacing: 3
                    Repeater {
                        model: Math.max(1, Math.floor(winding.width / 5))
                        Rectangle { width: 2; height: winding.height - 4; color: "#9A5420" }
                    }
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: label.implicitWidth + 4; height: label.implicitHeight; radius: 3
                    color: "#FFF4E0"
                    Text {
                        id: label
                        anchors.centerIn: parent
                        text: ({spule_1h: "1 H", spule_220uh: "220 µH", spule_1mh: "1 mH", spule_10mh: "10 mH"})[root.type] || "100 mH"
                        color: "#3A2410"
                        font.pixelSize: Math.round(9 * Theme.fontScale); font.bold: true
                    }
                }
            }
        }
    }

    // Fotowiderstand: runde Scheibe mit maeanderfoermiger Leiterbahn; Lichtschein je nach Helligkeit am Bauteil
    Component {
        id: photo
        Item {
            Rectangle { // Lichtschein (level: Beleuchtung 0..1, logarithmisch)
                anchors.centerIn: parent
                width: parent.height * 1.7; height: width; radius: width / 2
                color: "#FFF3A0"
                opacity: 0.15 + 0.6 * root.level
                visible: root.level > 0.2
            }
            Rectangle {
                anchors.centerIn: parent
                width: parent.height * 0.95; height: width; radius: width / 2
                color: "#D9A45B"; border.color: "#7A5420"; border.width: 2
                Shape {
                    anchors.fill: parent
                    preferredRendererType: Shape.CurveRenderer
                    ShapePath {
                        strokeColor: "#A3241B"; strokeWidth: 2; fillColor: "transparent"
                        PathSvg { path: "M8 9 H24 V13 H8 V17 H24 V21 H8 V25 H24" }
                    }
                }
            }
        }
    }

    // Heissleiter: Perle aus Lack (blau), wird bei Erwaermung rot (zusaetzlich das Zeichen ϑ)
    Component {
        id: thermistor
        Item {
            Rectangle {
                anchors.centerIn: parent
                width: parent.height * 0.9; height: parent.height * 0.75; radius: height / 2
                color: Qt.rgba(0.17 + 0.75 * root.level, 0.38 - 0.25 * root.level, 0.75 - 0.6 * root.level, 1)
                border.color: "#1E2B38"
                Text { anchors.centerIn: parent; text: "ϑ"; color: "white"; font.pixelSize: Math.round(14 * Theme.fontScale); font.bold: true }
            }
        }
    }

    // Elektromagnet: Kupferwicklung auf Eisenkern; eine Bueroklammer wird angezogen, sobald das Feld stark genug ist
    Component {
        id: magnet
        Item {
            id: em
            readonly property bool pulls: Math.abs(root.level) > 0.3 && !root.destroyed
            Rectangle { // Eisenkern
                anchors.centerIn: parent
                width: parent.width * 0.9; height: parent.height * 0.3
                color: "#7D8792"; border.color: "#3E4955"
            }
            Row { // Wicklung
                anchors.centerIn: parent
                spacing: 1
                Repeater {
                    model: 9
                    Rectangle { width: 4; height: em.height * 0.8; radius: 2; color: "#C8742A"; border.color: "#7A4012" }
                }
            }
            // Feldlinien bei Strom
            Repeater {
                model: em.pulls ? 2 : 0
                Rectangle {
                    required property int index
                    anchors.centerIn: parent
                    width: em.width * (0.95 + 0.25 * index); height: em.height * (1.3 + 0.5 * index)
                    radius: height / 2
                    color: "transparent"; border.color: "#5A8FD6"; border.width: 1
                    opacity: 0.6
                }
            }
            Text { // Bueroklammer (angezogen: am Kern)
                text: "📎"
                font.pixelSize: 14
                y: em.pulls ? -12 : -24
                x: em.width / 2 - 7
                Behavior on y { enabled: !Theme.reduceMotion; NumberAnimation { duration: 120 } }
            }
        }
    }

    // Relais auf quadratischer Platine: Fuesse A | B (Spule) oben, COM | NO (Schliesser) unten; Gehaeuse mit
    // sichtbarem Kontakt (geschlossen/offen, zusaetzlich Text)
    Component {
        id: relay
        Rectangle {
            id: rpcb
            anchors.fill: parent; radius: 6
            color: "#2F6B3A"; border.color: "#173D1E"; border.width: 2
            readonly property var f: root.feet.length === 4 ? root.feet
                                   : [Qt.point(16, 14), Qt.point(width - 16, 14),
                                      Qt.point(16, height - 14), Qt.point(width - 16, height - 14)]
            readonly property bool on: root.level > 0.5 && !root.destroyed
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.55; height: parent.height * 0.55; radius: 4
                color: "#2C5FA8"; border.color: "#16345E"
                Text { anchors.horizontalCenter: parent.horizontalCenter; y: 2; text: "K"; color: "white"; font.bold: true; font.pixelSize: Math.round(11 * Theme.fontScale) }
                Rectangle { // Kontaktzunge
                    width: parent.width * 0.5; height: 3; color: "#F2D16B"
                    x: parent.width * 0.25; y: parent.height * 0.7
                    transformOrigin: Item.Left
                    rotation: rpcb.on ? 0 : -25
                    Behavior on rotation { enabled: !Theme.reduceMotion; NumberAnimation { duration: 60 } }
                }
            }
            Repeater {
                model: ["A", "B", "COM", "NO"]
                Text {
                    required property int index
                    required property string modelData
                    text: modelData
                    font.pixelSize: Math.round(9 * Theme.fontScale); font.bold: true; color: "white"
                    x: rpcb.f[index].x + (rpcb.f[index].x < rpcb.width / 2 ? 7 : -7 - width)
                    y: rpcb.f[index].y + (rpcb.f[index].y < rpcb.height / 2 ? 4 : -4 - height)
                }
            }
        }
    }

    // Piezo-Schallgeber: schwarze Scheibe mit Plus-Zeichen, Schallringe beim Toenen
    Component {
        id: piezo
        Item {
            Rectangle {
                anchors.centerIn: parent
                width: parent.height * 0.95; height: width; radius: width / 2
                color: "#1E1E1E"; border.color: "#000000"
                Rectangle { anchors.centerIn: parent; width: 6; height: 6; radius: 3; color: "#555555" }
                PlusMark { x: 1; y: parent.height - height }
            }
            Repeater {
                model: root.level > 0.05 && !root.destroyed ? 2 : 0
                Rectangle {
                    required property int index
                    anchors.centerIn: parent
                    width: parent.height * (1.25 + 0.3 * index); height: width; radius: width / 2
                    color: "transparent"; border.color: "#E8A33B"; border.width: 2
                    opacity: 0.7 - 0.25 * index
                }
            }
        }
    }

    // Tastkopf (ab Volta 2): Griff in der Kanalfarbe (K1 gelb, K2 tuerkis) mit Fahne »K1«/»K2«, Massezeichen fuer den
    // Bezug 0V; die Spitze (das eine Bein) zeigt nach unten ins Feld
    Component {
        id: scopeProbe
        Item {
            readonly property color tint: root.tag > 0 ? Theme.probeColors[(root.tag - 1) % Theme.probeColors.length] : "#8E9AA8"
            Rectangle {
                anchors.fill: parent
                radius: 8
                color: parent.tint
                border.color: "#1E2B38"; border.width: 2
            }
            Text {
                anchors.centerIn: parent
                text: "K" + (root.tag > 0 ? root.tag : "")
                color: "#1E2B38"
                font.pixelSize: Math.round(14 * Theme.fontScale); font.bold: true
            }
            Text { // Bezug 0V
                anchors.right: parent.right; anchors.rightMargin: 3; anchors.bottom: parent.bottom
                text: "⏚"; color: "#1E2B38"; font.pixelSize: 11
            }
        }
    }

    // Messstelle (ab Volta 2): graues Gehaeuse mit Messgroesse im Kreis (V, A, Ω) und Nummer (Ziffer und Farbe);
    // Pluszeichen am ersten Anschluss. Den Messwert schreibt PartItem unter das Bauteil.
    Component {
        id: measurePoint
        Rectangle {
            radius: 6
            color: "#4A5A6E"; border.color: "#25303D"; border.width: 1
            PlusMark { x: 1; y: parent.height - height }
            Rectangle {
                id: dial
                anchors.centerIn: parent
                width: parent.height - 6; height: width; radius: width / 2
                color: "white"; border.width: 3
                // Rand in der Farbe der Nummer (Konzept 9.3: 1 blau, 2 orange)
                border.color: root.tag > 0 ? Theme.measureTagColors[(root.tag - 1) % Theme.measureTagColors.length] : "#25303D"
                Text {
                    anchors.centerIn: parent
                    text: root.type === "messstelle_a" ? "A" : root.type === "messstelle_ohm" ? "Ω" : "V"
                    color: "#1E2B38"
                    font.pixelSize: Math.round(15 * Theme.fontScale); font.bold: true
                }
            }
            Rectangle {
                visible: root.tag > 0
                x: dial.x + dial.width - 6; y: 1
                width: 14; height: 14; radius: 7
                color: root.tag > 0 ? Theme.measureTagColors[(root.tag - 1) % Theme.measureTagColors.length] : "transparent"
                border.color: "white"; border.width: 1
                Text { anchors.centerIn: parent; text: root.tag; color: "white"; font.pixelSize: Math.round(9 * Theme.fontScale); font.bold: true }
            }
        }
    }

    // Feuchtesonde: Platinchen mit zwei Kammzinken; nass: Wasserfilm dazwischen (Wassertropfen zusaetzlich)
    Component {
        id: probe
        Rectangle {
            radius: 4
            color: "#2F6B3A"; border.color: "#173D1E"
            Rectangle { x: parent.width * 0.2; y: 4; width: 4; height: parent.height - 8; color: "#D9B44A" }
            Rectangle { x: parent.width * 0.8 - 4; y: 4; width: 4; height: parent.height - 8; color: "#D9B44A" }
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.6; height: parent.height * 0.7; radius: 6
                color: "#5AA9E6"
                opacity: 0.75 * root.level
            }
            Text {
                visible: root.level > 0.3
                anchors.centerIn: parent
                text: "💧"
                font.pixelSize: 14
            }
        }
    }
}
