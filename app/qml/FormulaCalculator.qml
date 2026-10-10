// Formelrechner (Konzept 5.7, ab Stufe 3): Rechenblaetter, in die das Kind die bekannten Groessen eintraegt und die
// gesuchte erhaelt - mit Rechenweg in einer Zeile, damit das Ergebnis kein Orakel ist. Eingaben mit Vorsatz
// (4,7k, 470m, 100µ). Dazu ein gewoehnlicher Rechenmodus mit Tastenfeld. Gerechnet wird im Modell (model/formula).
// Rechenblaetter der Stufe 4 (Spannungsteiler, Zeitkonstante, Kippstufe) sind bis dahin mit Schloss sichtbar.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Flickable {
    id: calc
    required property var controller
    property bool canApply: false      // ausgewaehlter Widerstand im Labor
    property string selectedPartId: ""
    property string sheet: "ohm"
    readonly property int stage: controller.unlockedStage

    contentHeight: col.implicitHeight + 20
    clip: true

    readonly property var sheets: [
        {id: "ohm", name: qsTr("Ohmsches Gesetz"), stage: 3},
        {id: "led", name: qsTr("Vorwiderstand LED"), stage: 3},
        {id: "network", name: qsTr("Reihe und parallel"), stage: 3},
        {id: "power", name: qsTr("Leistung"), stage: 3},
        {id: "divider", name: qsTr("Spannungsteiler"), stage: 4},
        {id: "tau", name: qsTr("Zeitkonstante"), stage: 4},
        {id: "astable", name: qsTr("Kippstufe"), stage: 4},
        {id: "plain", name: qsTr("Rechner"), stage: 3}
    ]
    function fmt(v, unit) { return calc.controller.formulaFormat(v, unit) }
    function val(field) { const p = calc.controller.formulaParse(field.text); return p.ok ? p.value : 0 }
    function okOrEmpty(field) { return field.text.trim() === "" || calc.controller.formulaParse(field.text).ok }

    component Body: Label {
        width: parent ? parent.width : 300
        wrapMode: Text.Wrap
        font.pixelSize: Math.round(14 * Theme.fontScale)
        color: "#1E2B38"
    }
    // Eingabefeld fuer eine Groesse: Formelzeichen, Text, Einheit; ungueltige Eingabe rot umrandet (und Hinweis)
    component Quantity: Row {
        property string symbol
        property string unit
        property alias text: tf.text
        property alias field: tf
        property string hint: ""
        spacing: 8
        height: Theme.control
        Label { width: 40; anchors.verticalCenter: parent.verticalCenter; text: parent.symbol; font.bold: true; font.pixelSize: Math.round(15 * Theme.fontScale); color: Theme.navy }
        TextField {
            id: tf
            width: 180; height: Theme.control
            placeholderText: parent.hint
            inputMethodHints: Qt.ImhPreferNumbers
            font.pixelSize: Math.round(15 * Theme.fontScale)
            background: Rectangle {
                radius: 6
                color: "white"
                border.color: calc.okOrEmpty(tf) ? Theme.fieldOutline : "#C62828"
                border.width: calc.okOrEmpty(tf) ? 1 : 3
            }
        }
        Label { anchors.verticalCenter: parent.verticalCenter; text: parent.unit; font.pixelSize: Math.round(15 * Theme.fontScale); color: Theme.navy }
    }
    // Ergebnis gross, darunter der Rechenweg
    component Result: Rectangle {
        property string value
        property string way
        property bool warn: false
        width: parent ? parent.width : 300
        height: resCol.implicitHeight + 16
        radius: 8
        color: warn ? "#FFF0E0" : "#EAF4EA"
        border.color: warn ? Theme.signalOrange : "#2E7D32"
        visible: value !== ""
        Column {
            id: resCol
            x: 10; y: 8
            width: parent.width - 20
            spacing: 4
            Label { width: parent.width; text: parent.parent.value; wrapMode: Text.Wrap; font.bold: true; font.pixelSize: Math.round(17 * Theme.fontScale); color: Theme.navy }
            Label { width: parent.width; text: parent.parent.way; wrapMode: Text.Wrap; font.pixelSize: Math.round(13 * Theme.fontScale); color: "#3E5266"; visible: text !== "" }
        }
    }

    Column {
        id: col
        x: 12; y: 10
        width: calc.width - 24
        spacing: 10

        Body {
            visible: calc.stage < 3
            text: "🔒 " + qsTr("Der Formelrechner wird mit Stufe 3 freigeschaltet.")
        }

        // Auswahl der Rechenblaetter
        Flow {
            width: parent.width
            spacing: 6
            visible: calc.stage >= 3
            Repeater {
                model: calc.sheets
                Button {
                    required property var modelData
                    objectName: "sheet_" + modelData.id
                    readonly property bool locked: calc.stage < modelData.stage
                    enabled: !locked
                    height: Theme.control
                    text: (locked ? "🔒 " : "") + modelData.name
                    font.pixelSize: Math.round(13 * Theme.fontScale)
                    font.bold: calc.sheet === modelData.id
                    onClicked: calc.sheet = modelData.id
                    background: Rectangle {
                        radius: 6
                        color: calc.sheet === modelData.id ? Theme.accent : parent.locked ? "#E2E5E8" : "#EEF4FA"
                        border.color: Theme.accent
                    }
                    palette.buttonText: calc.sheet === modelData.id ? "white" : Theme.navy
                }
            }
        }

        // ---------------------------------------------------------- Ohmsches Gesetz
        Column {
            visible: calc.stage >= 3 && calc.sheet === "ohm"
            width: parent.width
            spacing: 8
            Body { text: qsTr("Trage zwei Größen ein, lass die gesuchte leer.") }
            // Dreieck: U oben, R und I unten; die gesuchte Groesse ist hervorgehoben (abgedeckt)
            Item {
                width: parent.width; height: 96
                readonly property string missing: ou.text.trim() === "" ? "U" : orr.text.trim() === "" ? "R" : oi.text.trim() === "" ? "I" : ""
                Shape {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 130; height: 96
                    ShapePath {
                        strokeColor: Theme.navy; strokeWidth: 2; fillColor: "#F4F8FC"
                        PathSvg { path: "M65 2 L128 94 L2 94 Z M30 52 H100 M65 52 V94" }
                    }
                }
                Repeater {
                    model: [{s: "U", x: 0, y: 22}, {s: "R", x: -26, y: 66}, {s: "I", x: 26, y: 66}]
                    Label {
                        required property var modelData
                        x: parent.width / 2 + modelData.x - width / 2
                        y: modelData.y
                        text: modelData.s
                        font.bold: true
                        font.pixelSize: parent.missing === modelData.s ? 22 : 18
                        color: parent.missing === modelData.s ? Theme.signalOrange : Theme.navy
                    }
                }
            }
            Quantity { id: ou; symbol: "U"; unit: "V"; hint: "9" }
            Quantity { id: orr; symbol: "R"; unit: "Ω"; hint: "1k" }
            Quantity { id: oi; symbol: "I"; unit: "A"; hint: "9m" }
            Result {
                readonly property var r: calc.controller.formulaSheet("ohm", {U: calc.val(ou), R: calc.val(orr), I: calc.val(oi)})
                readonly property bool uMissing: ou.text.trim() === ""
                readonly property bool rMissing: orr.text.trim() === ""
                value: !r.ok ? "" : uMissing ? "U = " + calc.fmt(r.U, "V") : rMissing ? "R = " + calc.fmt(r.R, "Ω") : "I = " + calc.fmt(r.I, "A")
                way: !r.ok ? "" : uMissing ? qsTr("U = R · I = %1 · %2").arg(calc.fmt(r.R, "Ω")).arg(calc.fmt(r.I, "A"))
                               : rMissing ? qsTr("R = U ÷ I = %1 ÷ %2").arg(calc.fmt(r.U, "V")).arg(calc.fmt(r.I, "A"))
                               : qsTr("I = U ÷ R = %1 ÷ %2").arg(calc.fmt(r.U, "V")).arg(calc.fmt(r.R, "Ω"))
            }
        }

        // ---------------------------------------------------------- Vorwiderstand fuer Leuchtdioden
        Column {
            visible: calc.stage >= 3 && calc.sheet === "led"
            width: parent.width
            spacing: 8
            Quantity { id: lub; symbol: "Ub"; unit: "V"; text: "9" }
            Quantity { id: luf; symbol: "Uf"; unit: "V"; text: "1,9" }
            Row {
                spacing: 6
                Repeater {
                    model: [{n: qsTr("rot"), v: "1,9"}, {n: qsTr("gelb"), v: "2,0"}, {n: qsTr("grün"), v: "2,1"}, {n: qsTr("blau"), v: "3,1"}]
                    Button {
                        required property var modelData
                        height: 40
                        text: modelData.n
                        font.pixelSize: Math.round(12 * Theme.fontScale)
                        onClicked: luf.text = modelData.v
                    }
                }
            }
            Quantity { id: li; symbol: "I"; unit: "A"; text: "15m" }
            Result {
                id: ledRes
                readonly property var r: calc.controller.formulaSheet("led", {Ub: calc.val(lub), Uf: calc.val(luf), I: calc.val(li)})
                warn: r.tooHot === true
                value: !r.ok ? "" : qsTr("R = %1 → Normwert %2").arg(calc.fmt(r.R, "Ω")).arg(calc.fmt(r.norm, "Ω"))
                way: !r.ok ? "" : qsTr("R = (Ub − Uf) ÷ I = (%1 − %2) ÷ %3. Mit %4 fließen %5, der Widerstand wird mit %6 belastet.")
                                  .arg(calc.fmt(calc.val(lub), "V")).arg(calc.fmt(calc.val(luf), "V")).arg(calc.fmt(calc.val(li), "A"))
                                  .arg(calc.fmt(r.norm, "Ω")).arg(calc.fmt(r.normI, "A")).arg(calc.fmt(r.P, "W"))
                                  + (r.tooHot ? " " + qsTr("Achtung: mehr als 0,25 W – der Widerstand würde zu heiß.") : "")
            }
            Button {
                objectName: "ledApply"
                visible: ledRes.r.ok === true
                enabled: calc.canApply
                height: Theme.control
                text: calc.canApply ? qsTr("Normwert auf %1 übernehmen").arg(calc.selectedPartId) : qsTr("Erst einen Widerstand auswählen")
                onClicked: calc.controller.setPartValue(calc.selectedPartId, ledRes.r.norm)
            }
        }

        // ---------------------------------------------------------- Reihe und parallel
        Column {
            visible: calc.stage >= 3 && calc.sheet === "network"
            width: parent.width
            spacing: 8
            Body { text: qsTr("Zwei oder drei Widerstände: hintereinander (Reihe) und nebeneinander (parallel).") }
            Quantity { id: n1; symbol: "R1"; unit: "Ω"; text: "1k" }
            Quantity { id: n2; symbol: "R2"; unit: "Ω"; text: "1k" }
            Quantity { id: n3; symbol: "R3"; unit: "Ω"; hint: qsTr("leer") }
            Result {
                readonly property var r: calc.controller.formulaSheet("network", {R1: calc.val(n1), R2: calc.val(n2), R3: calc.val(n3)})
                value: !r.ok ? "" : qsTr("Reihe: %1 · parallel: %2").arg(calc.fmt(r.series, "Ω")).arg(calc.fmt(r.parallel, "Ω"))
                way: !r.ok ? "" : qsTr("Reihe: R = R1 + R2 + … · parallel: 1/R = 1/R1 + 1/R2 + … (parallel immer kleiner als der kleinste)")
            }
        }

        // ---------------------------------------------------------- Leistung
        Column {
            visible: calc.stage >= 3 && calc.sheet === "power"
            width: parent.width
            spacing: 8
            Body { text: qsTr("Zwei der drei Größen genügen.") }
            Quantity { id: pu; symbol: "U"; unit: "V"; text: "9" }
            Quantity { id: pi; symbol: "I"; unit: "A"; text: "20m" }
            Quantity { id: pr; symbol: "R"; unit: "Ω"; hint: qsTr("leer") }
            Result {
                readonly property var r: calc.controller.formulaSheet("power", {U: calc.val(pu), I: calc.val(pi), R: calc.val(pr)})
                warn: r.tooHot === true
                value: !r.ok ? "" : "P = " + calc.fmt(r.P, "W")
                way: !r.ok ? "" : (calc.val(pu) > 0 && calc.val(pi) > 0 ? qsTr("P = U · I")
                                   : calc.val(pr) > 0 && calc.val(pi) > 0 ? qsTr("P = I · I · R") : qsTr("P = U · U ÷ R"))
                                  + (r.tooHot ? " – " + qsTr("mehr als 0,25 W: zu viel für die Widerstände im Kasten.") : "")
            }
        }

        // ---------------------------------------------------------- Spannungsteiler (Stufe 4)
        Column {
            visible: calc.stage >= 4 && calc.sheet === "divider"
            width: parent.width
            spacing: 8
            Quantity { id: due; symbol: "Ue"; unit: "V"; text: "9" }
            Quantity { id: d1; symbol: "R1"; unit: "Ω"; text: "2k" }
            Quantity { id: d2; symbol: "R2"; unit: "Ω"; text: "1k" }
            Result {
                readonly property var r: calc.controller.formulaSheet("divider", {Ue: calc.val(due), R1: calc.val(d1), R2: calc.val(d2)})
                value: !r.ok ? "" : "Ua = " + calc.fmt(r.Ua, "V")
                way: !r.ok ? "" : qsTr("Ua = Ue · R2 ÷ (R1 + R2) = %1 · %2 ÷ %3 (ohne Last)")
                                  .arg(calc.fmt(calc.val(due), "V")).arg(calc.fmt(calc.val(d2), "Ω")).arg(calc.fmt(calc.val(d1) + calc.val(d2), "Ω"))
            }
        }

        // ---------------------------------------------------------- Zeitkonstante (Stufe 4)
        Column {
            visible: calc.stage >= 4 && calc.sheet === "tau"
            width: parent.width
            spacing: 8
            Quantity { id: tr; symbol: "R"; unit: "Ω"; text: "10k" }
            Quantity { id: tc; symbol: "C"; unit: "F"; text: "100µ" }
            Result {
                id: tauRes
                readonly property var r: calc.controller.formulaSheet("tau", {R: calc.val(tr), C: calc.val(tc)})
                value: !r.ok ? "" : "τ = " + calc.fmt(r.tau, "s")
                way: !r.ok ? "" : qsTr("τ = R · C. Nach τ ist der Kondensator zu 63 % geladen, nach 5 τ = %1 praktisch voll.").arg(calc.fmt(5 * r.tau, "s"))
            }
            // Ladekurve 0 .. 5 tau mit Marke bei tau (63 %)
            Item {
                id: tauChart
                visible: tauRes.r.ok === true
                width: parent.width; height: 110
                Rectangle { anchors.fill: parent; color: "#FAFBFC"; border.color: "#C9D1D9" }
                Shape {
                    anchors.fill: parent
                    ShapePath {
                        strokeColor: "#D32F2F"; strokeWidth: 2.5; fillColor: "transparent"
                        PathPolyline {
                            path: {
                                const c = tauRes.r.curve || []
                                const out = []
                                for (let k = 0; k < c.length; ++k)
                                    out.push(Qt.point(tauChart.width * k / (c.length - 1), 100 - 90 * c[k]))
                                return out
                            }
                        }
                    }
                    ShapePath {
                        strokeColor: "#8A99A8"; strokeWidth: 1; fillColor: "transparent"; strokeStyle: ShapePath.DashLine
                        PathSvg { path: "M" + (tauChart.width / 5) + " 0 V100 M0 " + (100 - 90 * 0.632) + " H" + tauChart.width }
                    }
                }
                Label { x: parent.width / 5 + 4; y: 2; text: "τ"; font.bold: true; color: "#3E5266" }
                Label { x: 4; y: 100 - 90 * 0.632 - 16; text: "63 %"; font.pixelSize: 11; color: "#3E5266" }
            }
        }

        // ---------------------------------------------------------- Kippstufe (Stufe 4)
        Column {
            visible: calc.stage >= 4 && calc.sheet === "astable"
            width: parent.width
            spacing: 8
            Body { text: qsTr("Symmetrische Kippstufe: beide Hälften mit gleichem R und C.") }
            Quantity { id: ar; symbol: "R"; unit: "Ω"; text: "22k" }
            Quantity { id: ac; symbol: "C"; unit: "F"; text: "22µ" }
            Result {
                readonly property var r: calc.controller.formulaSheet("astable", {R: calc.val(ar), C: calc.val(ac)})
                value: !r.ok ? "" : qsTr("f = %1 (alle %2 ein Blink)").arg(calc.fmt(r.f, "Hz")).arg(calc.fmt(r.T, "s"))
                way: !r.ok ? "" : qsTr("Jede Hälfte dauert 0,69 · R · C = %1; f = 1 ÷ (2 · 0,69 · R · C)").arg(calc.fmt(r.half, "s"))
            }
        }

        // ---------------------------------------------------------- Rechenmodus mit Tastenfeld
        Column {
            visible: calc.stage >= 3 && calc.sheet === "plain"
            width: parent.width
            spacing: 8
            TextField {
                id: expr
                objectName: "calcExpression"
                width: parent.width; height: Theme.control
                font.pixelSize: Math.round(18 * Theme.fontScale)
                placeholderText: "4,7k × 2m"
                horizontalAlignment: Text.AlignRight
            }
            Label {
                objectName: "calcResult"
                width: parent.width
                horizontalAlignment: Text.AlignRight
                readonly property var r: calc.controller.formulaEval(expr.text)
                text: expr.text.trim() === "" ? " " : r.ok ? "= " + calc.fmt(r.value, "") : qsTr("Eingabe prüfen")
                font.pixelSize: Math.round(20 * Theme.fontScale)
                font.bold: true
                color: r.ok || expr.text.trim() === "" ? Theme.navy : "#C62828"
            }
            Grid {
                columns: 5
                spacing: 6
                Repeater {
                    model: ["7", "8", "9", "÷", "⌫",
                            "4", "5", "6", "×", "(",
                            "1", "2", "3", "−", ")",
                            "0", ",", "k", "+", "C",
                            "M", "m", "µ", "n", "p"]
                    Button {
                        required property string modelData
                        width: Math.max(Theme.control, (col.width - 24) / 5); height: Theme.control
                        text: modelData
                        font.pixelSize: Math.round(17 * Theme.fontScale)
                        // Loeschtaste gezeichnet (das Zeichen fehlt auf Android)
                        contentItem: Item {
                            Label {
                                anchors.centerIn: parent
                                visible: modelData !== "⌫"
                                text: modelData
                                font: parent.parent.font
                                color: Theme.navy
                            }
                            DrawnIcon { anchors.centerIn: parent; visible: modelData === "⌫"; kind: "backspace"; color: Theme.navy; size: 22 }
                        }
                        Accessible.name: modelData === "⌫" ? qsTr("Zeichen löschen") : modelData
                        onClicked: {
                            if (modelData === "C") expr.text = ""
                            else if (modelData === "⌫") expr.text = expr.text.slice(0, -1)
                            else expr.text += modelData
                        }
                    }
                }
            }
            Body {
                text: qsTr("Vorsätze: p = billionstel, n = milliardstel, µ = millionstel, m = tausendstel, k = tausend, M = Million.")
                color: "#4A5866"
                font.pixelSize: Math.round(12 * Theme.fontScale)
            }
        }
    }
}
