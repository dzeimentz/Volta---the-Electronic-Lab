// Formelrechner (Konzept 5.7, ab Stufe 3): Rechenblaetter, in die das Kind die bekannten Groessen eintraegt und die
// gesuchte erhaelt - mit Rechenweg in einer Zeile, damit das Ergebnis kein Orakel ist. Eingaben mit Vorsatz
// (4,7k, 470m, 100µ). Dazu ein gewoehnlicher Rechenmodus mit Tastenfeld. Gerechnet wird im Modell (model/formula).
// Rechenblaetter der Stufe 4 (Spannungsteiler, Zeitkonstante, Kippstufe) sind bis dahin mit Schloss sichtbar.
// Ab Volta 2 sind die Blaetter aus Volta 1 sofort offen; dazu kommen Grenzfrequenz, Effektivwert, Brummspannung,
// Verstaerkung und Timer 555, jeweils ab der Stufe, in der sie gebraucht werden (stage2; -1 = nur Volta 1).
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Flickable {
    id: calc
    required property var controller
    property bool canApply: false      // ausgewaehlter Widerstand im Labor
    property string selectedPartId: ""
    property string sheet: "ohm"
    // ab Volta 3 sind alle Blaetter der Grundbaende (Volta 1 und 2) sofort offen
    readonly property int stage: controller.band === "" || controller.band === "volta1" || controller.band === "volta2"
                                 ? controller.unlockedStage : 99
    readonly property bool v2: controller.band !== "volta1" // Baende ab Volta 2
    readonly property bool v3: controller.band === "volta3"
    readonly property int firstStage: v2 ? 0 : 3
    // stage3: Blaetter, die es erst ab Volta 3 gibt (in den Grundbaenden -1)
    function sheetStage(s) { return v3 && s.stage3 !== undefined ? s.stage3 : v2 ? s.stage2 : s.stage }

    contentHeight: col.implicitHeight + 20
    clip: true

    readonly property var sheets: [
        {id: "ohm", name: qsTr("Ohmsches Gesetz"), stage: 3, stage2: 0},
        {id: "led", name: qsTr("Vorwiderstand LED"), stage: 3, stage2: 0},
        {id: "network", name: qsTr("Reihe und parallel"), stage: 3, stage2: 0},
        {id: "power", name: qsTr("Leistung"), stage: 3, stage2: 0},
        {id: "divider", name: qsTr("Spannungsteiler"), stage: 4, stage2: 0},
        {id: "tau", name: qsTr("Zeitkonstante"), stage: 4, stage2: 0},
        {id: "astable", name: qsTr("Kippstufe"), stage: 4, stage2: 0},
        {id: "cutoff", name: qsTr("Grenzfrequenz"), stage: -1, stage2: 2},
        {id: "rms", name: qsTr("Effektivwert"), stage: -1, stage2: 2},
        {id: "ripple", name: qsTr("Brummspannung"), stage: -1, stage2: 3},
        {id: "gainT", name: qsTr("Verstärkung Transistor"), stage: -1, stage2: 4},
        {id: "gainOp", name: qsTr("Verstärkung OP"), stage: -1, stage2: 5},
        {id: "t555", name: qsTr("Timer 555"), stage: -1, stage2: 6},
        {id: "thomson", name: qsTr("Schwingkreis"), stage: -1, stage2: -1, stage3: 0},
        {id: "quality", name: qsTr("Bandbreite und Güte"), stage: -1, stage2: -1, stage3: 0},
        {id: "efficiency", name: qsTr("Wirkungsgrad"), stage: -1, stage2: -1, stage3: 0},
        {id: "db", name: qsTr("Pegel in dB"), stage: -1, stage2: -1, stage3: 0},
        {id: "duty", name: qsTr("Tastverhältnis"), stage: -1, stage2: -1, stage3: 0},
        {id: "divide", name: qsTr("Frequenzteiler"), stage: -1, stage2: -1, stage3: 0},
        {id: "plain", name: qsTr("Rechner"), stage: 3, stage2: 0}
    ]
    // Auswahlknoepfe (Einweg/Bruecke, invertierend/nichtinvertierend, astabil/monostabil)
    component Choice: Row {
        id: choice
        property var options: []   // [{id, name}]
        property string current: ""
        spacing: 6
        Repeater {
            model: choice.options
            Button {
                required property var modelData
                height: Theme.control
                text: modelData.name
                font.pixelSize: Math.round(13 * Theme.fontScale)
                font.bold: choice.current === modelData.id
                onClicked: choice.current = modelData.id
                background: Rectangle {
                    radius: 6
                    color: choice.current === modelData.id ? Theme.accent : "#EEF4FA"
                    border.color: Theme.accent
                }
                palette.buttonText: choice.current === modelData.id ? "white" : Theme.navy
            }
        }
    }
    function fmt(v, unit) { return calc.controller.formulaFormat(v, unit) }
    function plain(v) { return String(Number(v.toPrecision(3))).replace("-", "−") } // Verhaeltnis ohne Vorsatz
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
            visible: calc.stage < calc.firstStage
            text: "🔒 " + qsTr("Der Formelrechner wird mit Stufe 3 freigeschaltet.")
        }

        // Auswahl der Rechenblaetter
        Flow {
            width: parent.width
            spacing: 6
            visible: calc.stage >= calc.firstStage
            Repeater {
                model: calc.sheets.filter(s => calc.sheetStage(s) >= 0)
                Button {
                    required property var modelData
                    objectName: "sheet_" + modelData.id
                    readonly property bool locked: calc.stage < calc.sheetStage(modelData)
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
            visible: calc.stage >= calc.firstStage && calc.sheet === "ohm"
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
            visible: calc.stage >= calc.firstStage && calc.sheet === "led"
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
                        height: Theme.control
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
            visible: calc.stage >= calc.firstStage && calc.sheet === "network"
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
            visible: calc.stage >= calc.firstStage && calc.sheet === "power"
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
            visible: calc.stage >= (calc.v2 ? 0 : 4) && calc.sheet === "divider"
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
            visible: calc.stage >= (calc.v2 ? 0 : 4) && calc.sheet === "tau"
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
            visible: calc.stage >= (calc.v2 ? 0 : 4) && calc.sheet === "astable"
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

        // ---------------------------------------------------------- Grenzfrequenz (ab Volta 2, Stufe 2)
        Column {
            visible: calc.v2 && calc.stage >= 2 && calc.sheet === "cutoff"
            width: parent.width
            spacing: 8
            Body { text: qsTr("RC-Glied (Tiefpass oder Hochpass) und Wien-Brücke: Bei dieser Frequenz wirken Widerstand und Kondensator gleich stark.") }
            Quantity { id: cr; symbol: "R"; unit: "Ω"; text: "10k" }
            Quantity { id: cc; symbol: "C"; unit: "F"; text: "10n" }
            Result {
                readonly property var r: calc.controller.formulaSheet("cutoff", {R: calc.val(cr), C: calc.val(cc)})
                value: !r.ok ? "" : "f = " + calc.fmt(r.f, "Hz")
                way: !r.ok ? "" : qsTr("f = 1 ÷ (2π · R · C) = 1 ÷ (6,28 · %1 · %2)").arg(calc.fmt(calc.val(cr), "Ω")).arg(calc.fmt(calc.val(cc), "F"))
            }
        }

        // ---------------------------------------------------------- Effektivwert (ab Volta 2, Stufe 2)
        Column {
            visible: calc.v2 && calc.stage >= 2 && calc.sheet === "rms"
            width: parent.width
            spacing: 8
            Body { text: qsTr("Für eine Sinusspannung: Der Effektivwert macht in einem Widerstand so viel Wärme wie eine gleich große Gleichspannung.") }
            Quantity { id: rus; symbol: "Us"; unit: "V"; text: "4,2" }
            Result {
                readonly property var r: calc.controller.formulaSheet("rms", {Us: calc.val(rus)})
                value: !r.ok ? "" : qsTr("Ueff = %1 · Uss = %2").arg(calc.fmt(r.eff, "V")).arg(calc.fmt(r.pp, "V"))
                way: !r.ok ? "" : qsTr("Ueff = Us ÷ 1,41 = %1 ÷ 1,41. Von Spitze zu Spitze ist es doppelt so viel wie Us.").arg(calc.fmt(calc.val(rus), "V"))
            }
        }

        // ---------------------------------------------------------- Brummspannung (ab Volta 2, Stufe 3)
        Column {
            visible: calc.v2 && calc.stage >= 3 && calc.sheet === "ripple"
            width: parent.width
            spacing: 8
            Body { text: qsTr("Wie weit sinkt die Spannung am Ladeelko zwischen zwei Nachladungen? Eine Näherung für kleine Brummspannungen.") }
            Choice {
                id: rmode
                current: "half"
                options: [{id: "half", name: qsTr("Einweg")}, {id: "bridge", name: qsTr("Brücke")}]
            }
            Quantity { id: ri; symbol: "I"; unit: "A"; text: "20m" }
            Quantity { id: rc; symbol: "C"; unit: "F"; text: "100µ" }
            Quantity { id: rf; symbol: "f"; unit: "Hz"; text: "50" }
            Result {
                readonly property var r: calc.controller.formulaSheet("ripple", {I: calc.val(ri), C: calc.val(rc), f: calc.val(rf), bridge: rmode.current === "bridge"})
                value: !r.ok ? "" : "ΔU ≈ " + calc.fmt(r.dU, "V")
                way: !r.ok ? "" : (rmode.current === "bridge"
                                   ? qsTr("ΔU ≈ I ÷ (2 · f · C) = %1 ÷ (2 · %2 · %3). Die Brücke lädt zweimal je Schwingung nach.")
                                   : qsTr("ΔU ≈ I ÷ (f · C) = %1 ÷ (%2 · %3). Einweg lädt einmal je Schwingung nach."))
                                  .arg(calc.fmt(calc.val(ri), "A")).arg(calc.fmt(calc.val(rf), "Hz")).arg(calc.fmt(calc.val(rc), "F"))
            }
        }

        // ---------------------------------------------------------- Verstaerkung Transistor (ab Volta 2, Stufe 4)
        Column {
            visible: calc.v2 && calc.stage >= 4 && calc.sheet === "gainT"
            width: parent.width
            spacing: 8
            Body { text: qsTr("Emitterschaltung mit Emitterwiderstand: Die Verstärkung hängt fast nur von RC und RE ab.") }
            Quantity { id: grc; symbol: "RC"; unit: "Ω"; text: "2,2k" }
            Quantity { id: gre; symbol: "RE"; unit: "Ω"; text: "470" }
            Quantity { id: gue; symbol: "Ue"; unit: "V"; hint: qsTr("leer") }
            Result {
                readonly property var r: calc.controller.formulaSheet("gain", {mode: "transistor", Rf: calc.val(grc), Rin: calc.val(gre), Ue: calc.val(gue)})
                value: !r.ok ? "" : "v ≈ " + calc.plain(r.v) + (calc.val(gue) > 0 ? " · Ua ≈ " + calc.fmt(Math.abs(r.Ua), "V") : "")
                way: !r.ok ? "" : qsTr("v ≈ −RC ÷ RE = −%1 ÷ %2. Das Minus heißt: Der Ausgang bewegt sich gegen den Eingang.").arg(calc.fmt(calc.val(grc), "Ω")).arg(calc.fmt(calc.val(gre), "Ω"))
            }
        }

        // ---------------------------------------------------------- Verstaerkung OP (ab Volta 2, Stufe 5)
        Column {
            visible: calc.v2 && calc.stage >= 5 && calc.sheet === "gainOp"
            width: parent.width
            spacing: 8
            Choice {
                id: gmode
                current: "inv"
                options: [{id: "inv", name: qsTr("invertierend")}, {id: "noninv", name: qsTr("nichtinvertierend")}]
            }
            Body {
                text: gmode.current === "inv" ? qsTr("Eingang über Rin an IN−, Rf vom Ausgang zurück an IN−, IN+ an 0V.")
                                              : qsTr("Eingang an IN+, Rf vom Ausgang an IN−, Rg von IN− an 0V.")
            }
            Quantity { id: grf; symbol: "Rf"; unit: "Ω"; text: "47k" }
            Quantity { id: grin; symbol: gmode.current === "inv" ? "Rin" : "Rg"; unit: "Ω"; text: "10k" }
            Quantity { id: gue2; symbol: "Ue"; unit: "V"; hint: qsTr("leer") }
            Result {
                readonly property var r: calc.controller.formulaSheet("gain", {mode: gmode.current, Rf: calc.val(grf), Rin: calc.val(grin), Ue: calc.val(gue2)})
                value: !r.ok ? "" : "v = " + calc.plain(r.v) + (calc.val(gue2) > 0 ? " · Ua = " + calc.fmt(r.Ua, "V") : "")
                way: !r.ok ? "" : (gmode.current === "inv" ? qsTr("v = −Rf ÷ Rin = −%1 ÷ %2") : qsTr("v = 1 + Rf ÷ Rg = 1 + %1 ÷ %2"))
                                  .arg(calc.fmt(calc.val(grf), "Ω")).arg(calc.fmt(calc.val(grin), "Ω"))
                                  + ". " + qsTr("Näher als etwa 1,5 V an die Versorgung kommt der Ausgang nicht.")
            }
        }

        // ---------------------------------------------------------- Timer 555 (ab Volta 2, Stufe 6)
        Column {
            visible: calc.v2 && calc.stage >= 6 && calc.sheet === "t555"
            width: parent.width
            spacing: 8
            Choice {
                id: tmode
                current: "astable"
                options: [{id: "astable", name: qsTr("astabil (Blinker)")}, {id: "mono", name: qsTr("monostabil (Zeitschalter)")}]
            }
            Quantity { id: t1; visible: tmode.current === "astable"; symbol: "R1"; unit: "Ω"; text: "1k" }
            Quantity { id: t2; visible: tmode.current === "astable"; symbol: "R2"; unit: "Ω"; text: "4,7k" }
            Quantity { id: tr1; visible: tmode.current === "mono"; symbol: "R"; unit: "Ω"; text: "47k" }
            Quantity { id: t3; symbol: "C"; unit: "F"; text: "100µ" }
            Result {
                readonly property var r: calc.controller.formulaSheet("t555", {R1: calc.val(t1), R2: calc.val(t2), R: calc.val(tr1), C: calc.val(t3)})
                value: tmode.current === "astable"
                       ? (!r.ok ? "" : qsTr("f = %1 · oben %2, unten %3").arg(calc.fmt(r.f, "Hz")).arg(calc.fmt(r.high, "s")).arg(calc.fmt(r.low, "s")))
                       : (!r.monoOk ? "" : "t = " + calc.fmt(r.mono, "s"))
                way: tmode.current === "astable"
                     ? (!r.ok ? "" : qsTr("f ≈ 1,44 ÷ ((R1 + 2 · R2) · C). Oben: 0,69 · (R1 + R2) · C, unten: 0,69 · R2 · C. Der Ausgang ist %1 % der Zeit oben.").arg(Math.round(r.duty * 100)))
                     : (!r.monoOk ? "" : qsTr("t ≈ 1,1 · R · C = 1,1 · %1 · %2").arg(calc.fmt(calc.val(tr1), "Ω")).arg(calc.fmt(calc.val(t3), "F")))
            }
        }

        // ---------------------------------------------------------- ab Volta 3: Schwingkreis (Thomson)
        Column {
            visible: calc.v3 && calc.sheet === "thomson"
            width: parent.width
            spacing: 8
            Body { text: qsTr("Spule und Kondensator bilden einen Schwingkreis. Bei der Resonanzfrequenz schwingt er am stärksten.") }
            Quantity { id: thl; symbol: "L"; unit: "H"; text: "220µ" }
            Quantity { id: thc; symbol: "C"; unit: "F"; text: "100p" }
            Result {
                readonly property var r: calc.controller.formulaSheet("thomson", {L: calc.val(thl), C: calc.val(thc)})
                value: !r.ok ? "" : "f₀ = " + calc.fmt(r.f, "Hz")
                way: !r.ok ? "" : qsTr("f₀ = 1 ÷ (2 · π · √(L · C)) = 1 ÷ (2 · π · √(%1 · %2)). Dort sind die Blindwiderstände von Spule und Kondensator gleich groß: je %3.")
                                  .arg(calc.fmt(calc.val(thl), "H")).arg(calc.fmt(calc.val(thc), "F")).arg(calc.fmt(r.X, "Ω"))
            }
        }

        // ---------------------------------------------------------- ab Volta 3: Bandbreite und Güte
        Column {
            visible: calc.v3 && calc.sheet === "quality"
            width: parent.width
            spacing: 8
            Body { text: qsTr("Lies am Frequenzgang-Schreiber die untere und die obere Grenzfrequenz ab.") }
            Quantity { id: qfu; symbol: "fu"; unit: "Hz"; text: "900" }
            Quantity { id: qfo; symbol: "fo"; unit: "Hz"; text: "1,1k" }
            Result {
                readonly property var r: calc.controller.formulaSheet("quality", {fu: calc.val(qfu), fo: calc.val(qfo)})
                value: !r.ok ? "" : qsTr("B = %1 · f₀ = %2 · Q = %3").arg(calc.fmt(r.B, "Hz")).arg(calc.fmt(r.f0, "Hz")).arg(calc.plain(r.Q))
                way: !r.ok ? "" : qsTr("B = fo − fu. f₀ = √(fu · fo). Q = f₀ ÷ B. Je größer die Güte, desto schmaler und spitzer die Kurve.")
            }
            Body {
                visible: calc.val(qfo) > 0 && calc.val(qfu) >= calc.val(qfo)
                text: qsTr("Die obere Grenzfrequenz fo muss größer sein als die untere fu.")
                color: "#C62828"
            }
        }

        // ---------------------------------------------------------- ab Volta 3: Wirkungsgrad
        Column {
            visible: calc.v3 && calc.sheet === "efficiency"
            width: parent.width
            spacing: 8
            Body { text: qsTr("Miss Spannung und Strom am Eingang und am Ausgang.") }
            Quantity { id: eue; symbol: "Ue"; unit: "V"; text: "9" }
            Quantity { id: eie; symbol: "Ie"; unit: "A"; text: "100m" }
            Quantity { id: eua; symbol: "Ua"; unit: "V"; text: "5" }
            Quantity { id: eia; symbol: "Ia"; unit: "A"; text: "100m" }
            Result {
                readonly property var r: calc.controller.formulaSheet("efficiency", {Ue: calc.val(eue), Ie: calc.val(eie), Ua: calc.val(eua), Ia: calc.val(eia)})
                warn: r.ok && r.impossible
                value: !r.ok ? "" : r.impossible ? qsTr("Heraus kommt mehr, als hineingeht. Prüfe deine Messung.")
                                                 : "η = " + Math.round(r.eta * 100) + " % · " + qsTr("Verlust %1").arg(calc.fmt(r.loss, "W"))
                way: !r.ok || r.impossible ? "" : qsTr("η = Pab ÷ Pzu = (%1 · %2) ÷ (%3 · %4) = %5 ÷ %6. Der Verlust wird zu Wärme.")
                                                  .arg(calc.fmt(calc.val(eua), "V")).arg(calc.fmt(calc.val(eia), "A"))
                                                  .arg(calc.fmt(calc.val(eue), "V")).arg(calc.fmt(calc.val(eie), "A"))
                                                  .arg(calc.fmt(r.Pout, "W")).arg(calc.fmt(r.Pin, "W"))
            }
        }

        // ---------------------------------------------------------- ab Volta 3: Pegel in dB
        Column {
            visible: calc.v3 && calc.sheet === "db"
            width: parent.width
            spacing: 8
            Choice {
                id: dbmode
                current: "forward"
                options: [{id: "forward", name: qsTr("Spannungen → dB")}, {id: "back", name: qsTr("dB → Spannung")}]
            }
            Quantity { id: dbue; symbol: "Ue"; unit: "V"; text: "1" }
            Quantity { id: dbua; visible: dbmode.current === "forward"; symbol: "Ua"; unit: "V"; text: "0,5" }
            Quantity { id: dbv; visible: dbmode.current === "back"; symbol: "dB"; unit: "dB"; text: "−3" }
            Result {
                readonly property var r: dbmode.current === "forward"
                                         ? calc.controller.formulaSheet("db", {mode: "forward", Ue: calc.val(dbue), Ua: calc.val(dbua)})
                                         : calc.controller.formulaSheet("db", dbEval.ok ? {mode: "back", dB: dbEval.value, Ue: calc.val(dbue)} : {mode: "back"})
                readonly property var dbEval: calc.controller.formulaEval(dbv.text)
                value: !r.ok ? "" : dbmode.current === "forward"
                                    ? calc.plain(Math.round(r.dB * 10) / 10 + 0) + " dB"
                                    : "Ua ÷ Ue = " + calc.plain(r.v) + (calc.val(dbue) > 0 ? " · Ua = " + calc.fmt(r.Ua, "V") : "")
                way: !r.ok ? "" : dbmode.current === "forward"
                                  ? qsTr("Pegel = 20 · lg(Ua ÷ Ue) = 20 · lg(%1). Minus heißt: Am Ausgang kommt weniger an.").arg(calc.plain(r.v))
                                  : qsTr("Ua ÷ Ue = 10 hoch (dB ÷ 20). +20 dB = zehnmal, +6 dB ≈ doppelt, −3 dB ≈ 71 %.")
            }
        }

        // ---------------------------------------------------------- ab Volta 3: Tastverhaeltnis
        Column {
            visible: calc.v3 && calc.sheet === "duty"
            width: parent.width
            spacing: 8
            Body { text: qsTr("Lies im Oszilloskop oder Logikanalysator ab, wie lange das Signal oben und unten ist.") }
            Quantity { id: dth; symbol: "t oben"; unit: "s"; text: "1m" }
            Quantity { id: dtl; symbol: "t unten"; unit: "s"; text: "3m" }
            Quantity { id: dtu; symbol: "U"; unit: "V"; hint: qsTr("leer") }
            Result {
                readonly property var r: calc.controller.formulaSheet("duty", {th: calc.val(dth), tl: calc.val(dtl)})
                value: !r.ok ? "" : qsTr("%1 % oben · f = %2").arg(Math.round(r.duty * 100)).arg(calc.fmt(r.f, "Hz"))
                                    + (calc.val(dtu) > 0 ? " · " + qsTr("Mittelwert %1").arg(calc.fmt(r.duty * calc.val(dtu), "V")) : "")
                way: !r.ok ? "" : qsTr("Tastverhältnis = t oben ÷ T, T = t oben + t unten = %1. f = 1 ÷ T.").arg(calc.fmt(r.T, "s"))
                                  + (calc.val(dtu) > 0 ? " " + qsTr("Mittelwert = Tastverhältnis · U.") : "")
            }
        }

        // ---------------------------------------------------------- ab Volta 3: Frequenzteiler
        Column {
            visible: calc.v3 && calc.sheet === "divide"
            width: parent.width
            spacing: 8
            Body { text: qsTr("Ein Zähler teilt die Frequenz am Takteingang.") }
            Quantity { id: dvf; symbol: "f"; unit: "Hz"; text: "1k" }
            Quantity { id: dvn; symbol: "n"; unit: ""; text: "2" }
            Result {
                readonly property var r: calc.controller.formulaSheet("divide", {f: calc.val(dvf), n: calc.val(dvn)})
                value: !r.ok ? "" : "f ÷ n = " + calc.fmt(r.out, "Hz")
                way: !r.ok ? "" : qsTr("Zähler: Q0 %1, Q1 %2, Q2 %3, Q3 %4 – jeder Ausgang halbiert. Dekadenzähler: jeder Ausgang %5.")
                                  .arg(calc.fmt(r.q[0], "Hz")).arg(calc.fmt(r.q[1], "Hz")).arg(calc.fmt(r.q[2], "Hz"))
                                  .arg(calc.fmt(r.q[3], "Hz")).arg(calc.fmt(r.decade, "Hz"))
            }
        }

        // ---------------------------------------------------------- Rechenmodus mit Tastenfeld
        Column {
            visible: calc.stage >= calc.firstStage && calc.sheet === "plain"
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
