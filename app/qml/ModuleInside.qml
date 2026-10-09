// »Blick hinein« (Konzept 5.4, Module): Module sind keine undurchschaubaren Zauberkaesten.
// - Teilschaltung (Blinkmodul): die innere Schaltung als Schaltplan mit laufender Simulation - welcher Transistor
//   leitet, wie weit die Kondensatoren geladen sind, ob der Ausgang schaltet.
// - Verhaltensmodell (Tonmodul): der Aufbau als Blockbild mit den Werten an den Anschluessen und einer Erklaerung,
//   warum hier nicht jede Schwingung einzeln gerechnet wird.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Popup {
    id: inside
    required property var controller
    property string moduleId
    property string moduleType
    property var info: ({})

    function show(id, type) {
        moduleId = id
        moduleType = type
        info = controller.moduleInside(id)
        open()
    }

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(parent ? parent.width - 40 : 820, 820)
    height: Math.min(parent ? parent.height - 40 : 600, 600)
    padding: 0
    background: Rectangle { radius: 12; color: "#FBF6EA"; border.color: Theme.navy; border.width: 2 }

    Connections {
        target: inside.controller
        enabled: inside.opened
        function onStateChanged() { inside.info = inside.controller.moduleInside(inside.moduleId) }
    }

    // Messwert eines inneren Bauteils (Teilschaltung)
    function item(name) {
        const list = info.items || []
        for (let i = 0; i < list.length; ++i) if (list[i].name === name) return list[i]
        return ({currentmA: 0, voltV: 0, on: false})
    }
    function num(v, digits) { return Number(v || 0).toLocaleString(Qt.locale(), "f", digits) }

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
            text: "🔍 " + qsTr("Blick hinein: %1 %2").arg(Texts.moduleName(inside.moduleType)).arg(inside.moduleId)
            color: "white"
            font.pixelSize: Math.round(18 * Theme.fontScale)
            font.bold: true
        }
        ToolButton {
            objectName: "insideClose"
            anchors.right: parent.right
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.control; height: Theme.control
            text: "✕"
            font.pixelSize: 18
            palette.buttonText: "white"
            onClicked: inside.close()
            Accessible.name: qsTr("Schließen")
        }
    }

    ScrollView {
        id: scroll
        anchors { left: parent.left; right: parent.right; top: header.bottom; bottom: parent.bottom; margins: 12 }
        contentWidth: availableWidth
        clip: true

        Column {
            width: scroll.availableWidth
            spacing: 12

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: Math.round(15 * Theme.fontScale)
                color: "#1E2B38"
                text: inside.moduleType === "tonmodul"
                      ? qsTr("Ein Ton schwingt Hunderte Male in jeder Sekunde – zu schnell, um jede Schwingung einzeln nachzurechnen. Darum arbeitet das Tonmodul in Volta mit einem Verhaltensmodell: An seinen Anschlüssen verhält es sich wie der echte Baustein, und den Ton spielt es direkt ab.")
                      : inside.moduleType === "zeitmodul"
                      ? qsTr("Im Zeitmodul leitet in Ruhe der Transistor T1. Ein Druck auf »start« schaltet T2 ein; dessen Sprung drückt über den Elko C die Basis von T1 ins Minus, T1 sperrt. Erst wenn sich C über R_zeit umgeladen hat – etwa 3 Sekunden –, leitet T1 wieder. Solange T1 sperrt, schaltet T3 den Ausgang. Was du hier siehst, wird gerade gerechnet.")
                      : inside.moduleType === "verstaerkermodul"
                      ? qsTr("Im Verstärkermodul arbeiten zwei Transistoren hintereinander: Der winzige Eingangsstrom steuert T1, und der rund 200-mal größere Strom von T1 steuert T2. Zusammen verstärken sie etwa 40 000-fach. Was du hier siehst, wird gerade gerechnet.")
                      : qsTr("Im Blinkmodul arbeiten zwei Transistoren gegeneinander: Immer leitet einer, der andere sperrt. Die Kondensatoren laden sich langsam um und kippen die Schaltung hin und her. Ein dritter Transistor schaltet den Ausgang. Was du hier siehst, wird gerade gerechnet.")
            }

            // ---------------------------------------------------------- Teilschaltung: Kippstufe
            Item {
                id: blink
                visible: inside.moduleType === "blinkmodul"
                width: parent.width
                height: visible ? 340 * k : 0
                readonly property real k: Math.min(1.25, width / 700)

                Item {
                    width: 700
                    height: 340
                    transformOrigin: Item.TopLeft
                    scale: blink.k

                    // Leitungen der Schaltung (Einheiten des Entwurfs)
                    Shape {
                        anchors.fill: parent
                        preferredRendererType: Shape.CurveRenderer
                        ShapePath {
                            strokeColor: Theme.navy
                            strokeWidth: 2
                            fillColor: "transparent"
                            capStyle: ShapePath.RoundCap
                            PathSvg {
                                path: "M30 20 H636 M30 300 H636"                 // plus oben, minus unten
                                      + " M280 20 V36 M280 68 V84 M200 84 H360"   // R_takt, Taktleitung
                                      + " M100 20 V36 M100 68 V200"               // RC1 und Kollektor T1
                                      + " M460 20 V36 M460 68 V200"               // RC2 und Kollektor T2
                                      + " M200 84 V100 M200 132 V216 H132"         // RB1 zur Basis T1
                                      + " M360 84 V100 M360 132 V216 H428"         // RB2 zur Basis T2
                                      + " M100 232 V300 M460 232 V300"             // Emitter
                                      + " M100 160 H248 M312 160 H360"             // C1: Kollektor T1 -> Basis T2
                                      + " M460 200 H312 M248 200 H200"             // C2: Kollektor T2 -> Basis T1
                                      + " M460 120 H496 M528 120 H580 V236 H588"        // RB3 zur Basis T3
                                      + " M620 220 V150 M620 252 V300"             // T3: aus und Emitter
                                      + " M280 84 H300"                            // takt nach aussen
                            }
                        }
                        ShapePath { // Verbindungspunkte
                            strokeColor: "transparent"
                            fillColor: Theme.navy
                            PathSvg {
                                path: {
                                    const pts = [[100, 20], [280, 20], [460, 20], [280, 84], [200, 84], [360, 84],
                                                 [100, 160], [360, 160], [200, 200], [460, 200], [460, 120],
                                                 [100, 300], [460, 300], [620, 300]]
                                    let s = ""
                                    for (const p of pts)
                                        s += "M" + (p[0] - 4) + " " + p[1] + " A4 4 0 1 0 " + (p[0] + 4) + " " + p[1]
                                             + " A4 4 0 1 0 " + (p[0] - 4) + " " + p[1] + " "
                                    return s
                                }
                            }
                        }
                    }

                    // Anschluesse des Moduls (wie auf der Modulleiste)
                    component Pin: Rectangle {
                        property string text
                        width: pinLabel.implicitWidth + 14
                        height: 24
                        radius: 12
                        color: Theme.field
                        border.color: Theme.fieldOutline
                        Label { id: pinLabel; anchors.centerIn: parent; text: parent.text; font.bold: true; color: Theme.navy; font.pixelSize: 13 }
                    }
                    Pin { x: 0; y: 8; text: Texts.pinName("plus") }
                    Pin { x: 0; y: 288; text: Texts.pinName("minus") }
                    Pin { x: 300; y: 72; text: Texts.pinName("takt") }
                    Pin { x: 600; y: 126; text: Texts.pinName("aus") }

                    // Widerstaende (senkrecht: um 90 Grad gedreht, Mitte auf der Leitung)
                    component VRes: Item {
                        property real cx
                        property real topY
                        property string name
                        SchematicSymbol { x: parent.cx - 32; y: parent.topY + 16; width: 64; height: 32; type: "widerstand"; rotation: 90 }
                        Label { x: parent.cx + 14; y: parent.topY + 6; text: parent.name; color: "#4A5866"; font.pixelSize: 12 }
                    }
                    VRes { cx: 280; topY: 20; name: "R takt" }
                    VRes { cx: 100; topY: 20; name: "RC1" }
                    VRes { cx: 460; topY: 20; name: "RC2" }
                    VRes { cx: 200; topY: 84; name: "RB1" }
                    VRes { cx: 360; topY: 84; name: "RB2" }
                    SchematicSymbol { x: 480; y: 104; width: 64; height: 32; type: "widerstand" }
                    Label { x: 496; y: 92; text: "RB3"; color: "#4A5866"; font.pixelSize: 12 }

                    // Kondensatoren mit Ladung (Spannung als Zahl und Balken)
                    component Cap: Item {
                        id: cap
                        property real cx
                        property real cy
                        property string name
                        property bool mirrored: false
                        property bool labelBelow: false
                        readonly property var reading: inside.item(name)
                        SchematicSymbol {
                            x: parent.cx - 32; y: parent.cy - 16; width: 64; height: 32; type: "elko"
                            transform: Scale { origin.x: 32; xScale: cap.mirrored ? -1 : 1 }
                        }
                        Label {
                            x: parent.cx - width / 2; y: parent.labelBelow ? parent.cy + 16 : parent.cy - 36
                            text: parent.name + "  " + qsTr("%1 V").arg(inside.num(parent.reading.voltV, 1))
                            color: Theme.navy
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }
                    Cap { cx: 280; cy: 160; name: "C1" }
                    Cap { cx: 280; cy: 200; name: "C2"; mirrored: true; labelBelow: true }

                    // Transistoren: leitet oder sperrt (Schein und Text, Farbe nie allein)
                    component Trans: Item {
                        id: trans
                        property real colX   // Kollektor x
                        property real colY   // Kollektor y
                        property string name
                        property bool mirrored: false
                        readonly property var reading: inside.item(name)
                        Rectangle {
                            x: parent.colX - 34; y: parent.colY - 18
                            width: 68; height: 68; radius: 34
                            color: parent.reading.on ? "#6648C774" : "transparent"
                            border.color: parent.reading.on ? "#2E7D32" : "transparent"
                            border.width: 2
                        }
                        SchematicSymbol {
                            x: parent.mirrored ? parent.colX - 24 : parent.colX - 40
                            y: parent.colY
                            width: 64; height: 32; type: "transistor_npn"
                            transform: Scale { origin.x: 32; xScale: trans.mirrored ? -1 : 1 }
                        }
                        Label {
                            x: parent.mirrored ? parent.colX - width - 30 : parent.colX + 28
                            y: parent.colY + 34
                            text: parent.name + "\n" + (parent.reading.on ? qsTr("leitet") : qsTr("sperrt"))
                                  + "\n" + qsTr("%1 mA").arg(inside.num(Math.abs(parent.reading.currentmA), 1))
                            color: parent.reading.on ? "#1B5E20" : "#4A5866"
                            font.pixelSize: 12
                            font.bold: parent.reading.on
                        }
                    }
                    Trans { colX: 100; colY: 200; name: "T1"; mirrored: true }
                    Trans { colX: 460; colY: 200; name: "T2" }
                    Trans { colX: 620; colY: 220; name: "T3" }
                }
            }

            // ---------------------------------------------------------- Teilschaltung: Bauteilliste mit Live-Werten
            Column {
                visible: inside.moduleType === "zeitmodul" || inside.moduleType === "verstaerkermodul"
                width: parent.width
                spacing: 4
                Repeater {
                    model: inside.info.items || []
                    Rectangle {
                        required property var modelData
                        readonly property bool transistor: modelData.name.startsWith("T")
                        readonly property bool cap: modelData.name === "C"
                        width: parent.width; height: 34; radius: 6
                        color: transistor && modelData.on ? "#E3F4E5" : "white"
                        border.color: transistor && modelData.on ? "#2E7D32" : Theme.fieldOutline
                        Row {
                            x: 10; anchors.verticalCenter: parent.verticalCenter
                            spacing: 12
                            SchematicSymbol {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 44; height: 22
                                type: parent.parent.transistor ? "transistor_npn" : parent.parent.cap ? "elko" : "widerstand"
                            }
                            Label { anchors.verticalCenter: parent.verticalCenter; width: 70; text: modelData.name; font.bold: true; color: Theme.navy }
                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                text: parent.parent.transistor ? (modelData.on ? qsTr("leitet") : qsTr("sperrt")) + " · " + qsTr("%1 mA").arg(inside.num(Math.abs(modelData.currentmA), 2))
                                    : parent.parent.cap ? qsTr("%1 V").arg(inside.num(modelData.voltV, 2))
                                    : qsTr("%1 mA").arg(inside.num(Math.abs(modelData.currentmA), 3))
                                color: parent.parent.transistor && modelData.on ? "#1B5E20" : "#3E5266"
                                font.bold: parent.parent.transistor && modelData.on
                            }
                        }
                    }
                }
            }

            // ---------------------------------------------------------- Verhaltensmodell: Blockbild
            Column {
                visible: inside.moduleType === "tonmodul"
                width: parent.width
                spacing: 10

                Row {
                    spacing: 6
                    component Block: Rectangle {
                        property string title
                        property string value
                        property bool lit: false
                        width: 132; height: 74; radius: 8
                        color: lit ? "#FFE9A8" : "white"
                        border.color: lit ? Theme.solderAmber : "#B9AE96"
                        border.width: lit ? 3 : 1
                        Column {
                            anchors.centerIn: parent
                            width: parent.width - 10
                            Label { width: parent.width; text: parent.parent.title; font.bold: true; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter; color: Theme.navy; font.pixelSize: Math.round(13 * Theme.fontScale) }
                            Label { width: parent.width; text: parent.parent.value; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter; color: "#1E2B38"; font.pixelSize: Math.round(12 * Theme.fontScale) }
                        }
                    }
                    component Arrow: Label { anchors.verticalCenter: parent.verticalCenter; text: "→"; font.pixelSize: 22; color: Theme.navy }
                    Block {
                        title: qsTr("Eingang „ein“")
                        value: qsTr("%1 V").arg(inside.num(inside.info.inV, 1))
                        lit: inside.info.playing === true
                    }
                    Arrow {}
                    Block {
                        title: qsTr("Schaltschwelle")
                        value: qsTr("ab 1,5 V: an")
                        lit: inside.info.playing === true
                    }
                    Arrow {}
                    Block {
                        title: qsTr("Melodie und Tongeber")
                        value: inside.info.playing === true
                               ? ((inside.info.frequencyHz || 0) > 0 ? qsTr("%1 Hz").arg(inside.num(inside.info.frequencyHz, 0)) : qsTr("Pause"))
                               : qsTr("still")
                        lit: (inside.info.frequencyHz || 0) > 0
                    }
                    Arrow {}
                    Block {
                        title: qsTr("Endstufe „aus“")
                        value: qsTr("%1 mA").arg(inside.num(Math.abs(inside.info.outmA || 0), 0))
                        lit: Math.abs(inside.info.outmA || 0) > 1
                    }
                }

                // Die Melodie: acht Toene, der gerade gespielte ist markiert
                Row {
                    spacing: 6
                    Label { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Melodie:"); font.bold: true; color: Theme.navy; font.pixelSize: Math.round(14 * Theme.fontScale) }
                    Repeater {
                        model: ["C", "E", "G", "C'", "G", "E", "C", "–"]
                        Rectangle {
                            required property string modelData
                            required property int index
                            readonly property bool now: inside.info.note === index
                            width: 40; height: 40; radius: 6
                            color: now ? Theme.solderAmber : "white"
                            border.color: now ? Theme.navy : "#B9AE96"
                            border.width: now ? 3 : 1
                            Label { anchors.centerIn: parent; text: parent.modelData; font.bold: parent.now; color: Theme.navy; font.pixelSize: 14 }
                        }
                    }
                }
                Label {
                    width: parent.width
                    wrapMode: Text.Wrap
                    color: "#4A5866"
                    font.pixelSize: Math.round(13 * Theme.fontScale)
                    text: qsTr("Versorgung: %1 V. Die Endstufe schaltet im Takt des Tons ein und aus; Volta rechnet mit dem Mittelwert des Stroms. Ein Lautsprecher zwischen Plus und „aus“ spielt die Melodie mit.").arg(inside.num(inside.info.supplyV, 1))
                }
            }
        }
    }
}
