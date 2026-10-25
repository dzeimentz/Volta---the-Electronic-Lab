import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Historische Entwicklung der Gatter mit Zetti: Zeitleiste von Aristoteles bis heute. Je Station erzaehlt Zetti, daneben
// eine Zeichnung, oft zum Anfassen (Schalter, Relais, Halbaddierer). Am Ende ein Ueberblick. Daten nachgeprueft; kindgerecht
// vereinfacht, aber nichts Falsches (Z1 mechanisch, Z3 mit Relais).
Rectangle {
    id: root
    signal backRequested()
    color: "#FBF6EA"
    objectName: "historyPage"

    property int current: 0
    function go(i) { current = Math.max(0, Math.min(stations.length - 1, i)) }

    readonly property var stations: [
        { key: "aristoteles", year: qsTr("um 350 v. Chr."), title: qsTr("Wahr oder falsch"), pose: "erklaerend",
          text: qsTr("Schon vor über 2000 Jahren fragte sich der griechische Denker Aristoteles: Nach welchen Regeln denken wir richtig? Er stellte fest: Eine Aussage ist entweder wahr oder falsch – etwas Drittes gibt es nicht. Genau so arbeiten Rechner bis heute."),
          where: qsTr("In Zuse 1: Eine Lampe ist an oder aus – wie wahr und falsch.") },
        { key: "leibniz", year: "1703", title: qsTr("Rechnen mit 0 und 1"), pose: "begeistert",
          text: qsTr("Gottfried Wilhelm Leibniz zeigte, dass man alle Zahlen nur mit den Ziffern 0 und 1 schreiben kann: das Zweiersystem. Damals ahnte niemand, dass Rechner einmal genau so zählen würden. Zähl selbst mit drei Lampen!"),
          where: qsTr("In Zuse 1: Stufe 3 »Zahlen und Rechnen«.") },
        { key: "boole", year: "1854", title: qsTr("Logik wird Mathematik"), pose: "zeigend",
          text: qsTr("George Boole schrieb »wahr« als 1 und »falsch« als 0 und rechnete damit wie mit Zahlen. Aus »Es regnet UND ich habe einen Schirm« wurde eine Rechnung. Das war reine Mathematik – mit Strom hatte es noch nichts zu tun."),
          where: qsTr("In Zuse 1: Die Tabellen in Stufe 1 sind Boolesche Rechnungen.") },
        { key: "peirce", year: "1880 / 1913", title: qsTr("Ein Baustein für alles"), pose: "nachdenklich",
          text: qsTr("Charles Sanders Peirce entdeckte um 1880: Mit einer einzigen Verknüpfung lassen sich alle anderen bauen. Er veröffentlichte es kaum. 1913 zeigte Henry Sheffer dasselbe für die Verknüpfung, die wir heute NAND nennen."),
          where: qsTr("In Zuse 1: Experiment 2.3 »Alles aus einem Baustein«.") },
        { key: "shannon", year: "1937", title: qsTr("Schalter können denken"), pose: "ueberrascht",
          text: qsTr("Der Student Claude Shannon entdeckte: Boolesche Logik passt genau auf Schalter! Zwei Schalter hintereinander – die Lampe leuchtet nur, wenn A UND B geschlossen sind. Nebeneinander reicht A ODER B. Ein Öffner dreht um: NICHT. Ähnliches fanden zur selben Zeit Forscher in Japan und der Sowjetunion. Probier es aus!"),
          where: qsTr("In Zuse 1: Stufe 0 und Stufe 1.") },
        { key: "zuse", year: "1938 / 1941", title: qsTr("Konrad Zuse baut Rechner"), pose: "froehlich",
          text: qsTr("In Berlin baute Konrad Zuse im Wohnzimmer seiner Eltern die Z1. Sie rechnete schon mit 0 und 1, aber mechanisch: mit Tausenden dünner Blechteile, die sich verschoben. 1941 lief seine Z3 mit rund 2000 Relais – der erste funktionierende frei programmierbare Rechner der Welt. Ein Relais ist ein Schalter, den ein Elektromagnet umlegt."),
          where: qsTr("Nach Konrad Zuse heißt diese App – und ich, Zetti, auch ein bisschen.") },
        { key: "eniac", year: "1946", title: qsTr("Röhren statt Relais"), pose: "ueberrascht",
          text: qsTr("In den USA startete der ENIAC mit fast 18 000 Elektronenröhren. Röhren schalten ohne bewegliche Teile und sind dadurch etwa tausendmal schneller als Relais. Aber sie werden heiß, brauchen viel Strom und brennen immer wieder durch."),
          where: "" },
        { key: "transistor", year: "1947", title: qsTr("Der Transistor"), pose: "begeistert",
          text: qsTr("John Bardeen, Walter Brattain und William Shockley erfanden in den Bell Labs den Transistor: einen Schalter ohne Mechanik und ohne Glühen – klein, kühl und langlebig. Seitdem werden Schalter immer kleiner und schneller. Schau, wie schnell sie umschalten können!"),
          where: "" },
        { key: "chip", year: qsTr("ab 1958"), title: qsTr("Vier NAND in einem Chip"), pose: "zeigend",
          text: qsTr("1958 brachte Jack Kilby mehrere Bauteile auf ein einziges Stück Halbleiter: den ersten Chip. Ab Mitte der 1960er Jahre wurde die 74er-Reihe von Texas Instruments zum Standard. Der berühmte 7400 enthält vier NAND-Gatter. Weil man aus NAND alles bauen kann, steckte er fast überall drin."),
          where: qsTr("In Zuse 1: Die Werkzeugkiste ist wie eine Kiste voller Chips.") },
        { key: "xoder", year: qsTr("1950er"), title: qsTr("Rechnen mit XODER"), pose: "bauen",
          text: qsTr("Rechenwerke brauchen ein besonderes Gatter: XODER, »entweder – oder«. Rechnet man 1 + 1 im Zweiersystem, kommt 10 heraus. XODER liefert die Stelle, UND den Übertrag. Das ist der Halbaddierer. XODER hilft auch, Fehler in Daten zu erkennen."),
          where: qsTr("In Zuse 1: Treppenlicht in Stufe 2, Halbaddierer in Stufe 3.") },
        { key: "cmos", year: "1968", title: qsTr("Strom sparen mit CMOS"), pose: "freundlich",
          text: qsTr("Mit den CMOS-Chips der 4000er-Reihe brauchen Gatter fast nur beim Umschalten Strom, im Ruhezustand beinahe keinen. Erst dadurch wurden Taschenrechner, Digitaluhren und später Handys möglich, die lange mit einer Batterie auskommen."),
          where: "" },
        { key: "fpga", year: "1985", title: qsTr("Gatter zum Umprogrammieren"), pose: "losgehts",
          text: qsTr("Bei programmierbaren Chips legt man selbst fest, welche Gatter wie verbunden sind – mit einem Programm statt mit dem Lötkolben. 1985 kam der erste FPGA auf den Markt. Tippe auf die Zellen und mach daraus andere Gatter!"),
          where: qsTr("In Zuse 1: Auf der Werkbank verbindest du Gatter selbst – fast wie in einem FPGA.") },
        { key: "heute", year: qsTr("heute"), title: qsTr("Milliarden Gatter"), pose: "begeistert",
          text: qsTr("Der Chip in einem Handy hat viele Milliarden Transistoren. Sie sind nur wenige Nanometer groß: Mehr als tausend passen nebeneinander in die Breite eines Haares. Und doch macht jedes Gatter darin genau das, was du in Zuse 1 entdeckst: UND, ODER, NICHT."),
          where: qsTr("In Zuse 1: Fang beim Lernpfad an – genau wie die Geschichte.") },
        { key: "ueberblick", year: "", title: qsTr("Überblick"), pose: "erklaerend",
          text: qsTr("Hier siehst du noch einmal alle Gatter: woher die Idee kommt, womit man sie zuerst gebaut hat und wofür man sie braucht."),
          where: "" }
    ]
    readonly property var station: stations[current]

    Keys.onLeftPressed: go(current - 1)
    Keys.onRightPressed: go(current + 1)
    onVisibleChanged: if (visible) forceActiveFocus()

    // ---------------------------------------------------------------- Zeichnungen
    component Lamp: Column { // Lampe: Farbe und Wort (nie nur Farbe)
        property bool on: false
        property string label: ""
        spacing: 4
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 44; height: 44; radius: 22
            color: parent.on ? "#FFD54A" : ZTheme.surfaceHigh
            border.width: 2
            border.color: parent.on ? "#B8860B" : ZTheme.holeStrong
            Text {
                anchors.centerIn: parent
                text: parent.parent.on ? "1" : "0"
                color: ZTheme.text
                font.pixelSize: 18; font.bold: true; font.family: ZTheme.mono
            }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: parent.label
            color: ZTheme.muted
            font.pixelSize: Math.round(13 * ZTheme.fontScale)
        }
    }
    component Toggle01: ZButton { // Schalter zum Antippen, zeigt 0/1
        property bool on: false
        property string name: ""
        text: name + " = " + (on ? "1" : "0")
        primary: on
        font.family: ZTheme.mono
    }
    component Caption: Text {
        Layout.fillWidth: true
        Layout.preferredWidth: 420 // langer Text soll umbrechen, nicht die Zeichnung verbreitern
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        color: ZTheme.muted
        font.pixelSize: Math.round(14 * ZTheme.fontScale)
    }

    component NandRow: RowLayout {
        property string name
        property int count: 1
        property string note
        Layout.alignment: Qt.AlignHCenter
        spacing: 10
        Text {
            Layout.preferredWidth: 90
            horizontalAlignment: Text.AlignRight
            text: parent.name + "  ="
            color: ZTheme.navy
            font.pixelSize: Math.round(17 * ZTheme.fontScale); font.bold: true
        }
        Repeater {
            model: parent.count
            delegate: GateShape {
                width: 64; height: 52
                symbol: "&"; inverted: true
                us: workbench.symbolStyle === "us"
                stroke: ZTheme.navy
                fontSize: 20
            }
        }
        Text {
            Layout.preferredWidth: 200
            text: parent.note
            wrapMode: Text.WordWrap
            color: ZTheme.muted
            font.pixelSize: Math.round(13 * ZTheme.fontScale)
        }
    }
    component Circuit: Rectangle {
        id: circuit
        property string kind: "reihe" // "reihe", "parallel", "oeffner"
        property bool a: false
        property bool b: false
        readonly property bool lit: kind === "reihe" ? a && b : kind === "parallel" ? a || b : !a
        Layout.preferredWidth: 196
        Layout.preferredHeight: 230
        radius: 12
        color: ZTheme.surface
        border.color: ZTheme.line
        onLitChanged: canvas.requestPaint()
        onAChanged: canvas.requestPaint()
        onBChanged: canvas.requestPaint()
        Text {
            x: 12; y: 8
            text: circuit.kind === "reihe" ? qsTr("hintereinander = UND")
                : circuit.kind === "parallel" ? qsTr("nebeneinander = ODER") : qsTr("Öffner = NICHT")
            color: ZTheme.navy
            font.pixelSize: Math.round(15 * ZTheme.fontScale); font.bold: true
        }
        Canvas {
            id: canvas
            anchors { fill: parent; topMargin: 34; bottomMargin: 40 }
            onPaint: {
                const c = getContext("2d")
                c.reset()
                const w = width, h = height
                c.lineWidth = 3
                c.strokeStyle = circuit.lit ? "#0E7490" : "#5B6B7C"
                // Batterie links, Lampe rechts, Leitung oben mit Schaltern, unten zurueck
                const top = h * 0.12, bot = h * 0.9, lx = 18, rx = w - 18, my = (top + bot) / 2
                function sw(x1, x2, y, closed) { // Schalter von x1 nach x2
                    c.beginPath(); c.arc(x1, y, 4, 0, 2 * Math.PI); c.stroke()
                    c.beginPath(); c.arc(x2, y, 4, 0, 2 * Math.PI); c.stroke()
                    c.beginPath(); c.moveTo(x1, y)
                    if (closed) c.lineTo(x2, y); else c.lineTo(x2 - 6, y - (x2 - x1) * 0.55)
                    c.stroke()
                }
                // Rueckleitung unten, links mit Batterie, rechts mit Lampe
                c.beginPath(); c.moveTo(lx, top); c.lineTo(lx, bot); c.lineTo(rx, bot); c.lineTo(rx, top); c.stroke()
                const a1 = w * 0.3, a2 = w * 0.5, b1 = w * 0.56, b2 = w * 0.76
                if (circuit.kind === "reihe") {
                    c.beginPath(); c.moveTo(lx, top); c.lineTo(a1, top); c.moveTo(a2, top); c.lineTo(b1, top)
                    c.moveTo(b2, top); c.lineTo(rx, top); c.stroke()
                    sw(a1, a2, top, circuit.a); sw(b1, b2, top, circuit.b)
                } else if (circuit.kind === "parallel") {
                    const y2 = top + (bot - top) * 0.42, j1 = w * 0.25, j2 = w * 0.8
                    c.beginPath(); c.moveTo(lx, top); c.lineTo(w * 0.38, top); c.moveTo(w * 0.62, top); c.lineTo(rx, top)
                    c.moveTo(j1, top); c.lineTo(j1, y2); c.lineTo(w * 0.38, y2)
                    c.moveTo(w * 0.62, y2); c.lineTo(j2, y2); c.lineTo(j2, top); c.stroke()
                    sw(w * 0.38, w * 0.62, top, circuit.a); sw(w * 0.38, w * 0.62, y2, circuit.b)
                } else {
                    c.beginPath(); c.moveTo(lx, top); c.lineTo(w * 0.38, top); c.moveTo(w * 0.62, top); c.lineTo(rx, top); c.stroke()
                    sw(w * 0.38, w * 0.62, top, !circuit.a) // Oeffner: gedrueckt = offen
                }
                // Batterie (langer Strich +, kurzer Strich -)
                c.fillStyle = "#FFFFFF"; c.fillRect(lx - 14, my - 10, 28, 20)
                c.fillStyle = "#14213D"
                c.fillRect(lx - 14, my - 6, 28, 4)
                c.fillRect(lx - 8, my + 4, 16, 4)
                // Lampe
                c.beginPath(); c.arc(rx, my, 15, 0, 2 * Math.PI)
                c.fillStyle = circuit.lit ? "#FFD54A" : "#E6EDF3"; c.fill(); c.stroke()
                c.beginPath(); c.moveTo(rx - 10, my - 10); c.lineTo(rx + 10, my + 10)
                c.moveTo(rx + 10, my - 10); c.lineTo(rx - 10, my + 10); c.stroke()
            }
        }
        RowLayout {
            anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 6 }
            spacing: 6
            ZButton {
                implicitWidth: 70
                text: "A " + (circuit.a ? "▼" : "▲")
                primary: circuit.a
                onClicked: circuit.a = !circuit.a
                Accessible.name: qsTr("Schalter A") + (circuit.a ? ", " + qsTr("gedrückt") : "")
            }
            ZButton {
                visible: circuit.kind !== "oeffner"
                implicitWidth: 70
                text: "B " + (circuit.b ? "▼" : "▲")
                primary: circuit.b
                onClicked: circuit.b = !circuit.b
                Accessible.name: qsTr("Schalter B") + (circuit.b ? ", " + qsTr("gedrückt") : "")
            }
            Text {
                text: circuit.lit ? "💡 " + qsTr("leuchtet") : qsTr("aus")
                color: circuit.lit ? ZTheme.ok : ZTheme.muted
                font.pixelSize: Math.round(14 * ZTheme.fontScale); font.bold: true
            }
        }
    }

    Component { // Aristoteles: Aussage wahr oder falsch
        id: visAristoteles
        ColumnLayout {
            spacing: 14
            property bool lit: true
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("»Die Lampe leuchtet.«")
                color: ZTheme.text
                font.pixelSize: Math.round(22 * ZTheme.fontScale); font.italic: true
            }
            Lamp { Layout.alignment: Qt.AlignHCenter; on: parent.lit; label: parent.lit ? qsTr("an") : qsTr("aus") }
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: verdict.implicitWidth + 32
                Layout.preferredHeight: 48
                radius: 24
                color: parent.lit ? ZTheme.okBg : ZTheme.badBg
                border.color: parent.lit ? ZTheme.ok : ZTheme.bad
                border.width: 2
                Text {
                    id: verdict
                    anchors.centerIn: parent
                    text: parent.parent.lit ? "✓ " + qsTr("wahr") : "✗ " + qsTr("falsch")
                    color: parent.parent.lit ? ZTheme.ok : ZTheme.bad
                    font.pixelSize: Math.round(20 * ZTheme.fontScale); font.bold: true
                }
            }
            ZButton {
                Layout.alignment: Qt.AlignHCenter
                objectName: "historyLampSwitch"
                text: qsTr("Lampe umschalten")
                onClicked: parent.lit = !parent.lit
            }
        }
    }

    Component { // Leibniz: Zaehlen mit drei Lampen
        id: visLeibniz
        ColumnLayout {
            spacing: 14
            property int n: 5
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 18
                Repeater {
                    model: [4, 2, 1]
                    delegate: Lamp {
                        required property int modelData
                        on: (parent.parent.n & modelData) !== 0
                        label: qsTr("%1er").arg(modelData)
                    }
                }
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: (parent.n >> 2 & 1) + "" + (parent.n >> 1 & 1) + (parent.n & 1) + "  =  " + parent.n
                color: ZTheme.navy
                font.pixelSize: 34; font.bold: true; font.family: ZTheme.mono
            }
            Caption { text: qsTr("Jede Lampe ist doppelt so viel wert wie die rechts daneben.") }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 10
                ZButton { objectName: "historyCount"; primary: true; text: "+1"; onClicked: parent.parent.n = (parent.parent.n + 1) % 8 }
                ZButton { text: qsTr("Auf 0"); onClicked: parent.parent.n = 0 }
            }
        }
    }

    Component { // Boole: Rechentabellen
        id: visBoole
        ColumnLayout {
            spacing: 12
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 18
                Repeater {
                    model: [{name: qsTr("UND"), rows: [["0", "0", "0"], ["0", "1", "0"], ["1", "0", "0"], ["1", "1", "1"]]},
                            {name: qsTr("ODER"), rows: [["0", "0", "0"], ["0", "1", "1"], ["1", "0", "1"], ["1", "1", "1"]]},
                            {name: qsTr("NICHT"), rows: [["0", "", "1"], ["1", "", "0"]]}]
                    delegate: Rectangle {
                        required property var modelData
                        width: 120
                        height: tcol.implicitHeight + 16
                        radius: 10
                        color: ZTheme.surface
                        border.color: ZTheme.line
                        Column {
                            id: tcol
                            anchors.horizontalCenter: parent.horizontalCenter
                            y: 8
                            spacing: 2
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: parent.parent.modelData.name
                                color: ZTheme.navy
                                font.pixelSize: Math.round(17 * ZTheme.fontScale); font.bold: true
                            }
                            Repeater {
                                model: parent.parent.modelData.rows
                                delegate: Text {
                                    required property var modelData
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: (modelData[1] === "" ? modelData[0] : modelData[0] + " " + modelData[1]) + "  →  " + modelData[2]
                                    color: ZTheme.text
                                    font.pixelSize: 17; font.family: ZTheme.mono
                                }
                            }
                        }
                    }
                }
            }
            Caption { text: qsTr("1 bedeutet wahr, 0 bedeutet falsch. Zum Beispiel UND: Nur 1 und 1 ergibt 1.") }
        }
    }

    Component { // Peirce/Sheffer: alles aus NAND
        id: visPeirce
        ColumnLayout {
            spacing: 10
            NandRow { name: qsTr("NICHT"); count: 1; note: qsTr("1 NAND, beide Eingänge verbunden") }
            NandRow { name: qsTr("UND"); count: 2; note: qsTr("NAND, danach ein NAND als NICHT") }
            NandRow { name: qsTr("ODER"); count: 3; note: qsTr("beide Eingänge umdrehen, dann NAND") }
            Caption { text: qsTr("Mit genug NAND-Gattern lässt sich jede Schaltung bauen – sogar ein ganzer Rechner.") }
        }
    }

    Component { // Shannon: Reihe = UND, parallel = ODER, Oeffner = NICHT
        id: visShannon
        ColumnLayout {
            spacing: 12
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 12
                Circuit { objectName: "shannonSeries"; kind: "reihe" }
                Circuit { kind: "parallel" }
                Circuit { kind: "oeffner" }
            }
            Caption { text: qsTr("▼ = Taster gedrückt (1), ▲ = losgelassen (0). Der Öffner unterbricht den Strom, wenn man ihn drückt.") }
        }
    }

    Component { // Zuse: Relais
        id: visRelay
        ColumnLayout {
            id: relay
            spacing: 12
            property bool on: false
            onOnChanged: relayCanvas.requestPaint()
            Canvas {
                id: relayCanvas
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 420
                Layout.preferredHeight: 220
                onPaint: {
                    const c = getContext("2d")
                    c.reset()
                    c.lineWidth = 3
                    c.font = "bold 14px sans-serif"
                    // Spule (Elektromagnet) unten links
                    c.fillStyle = relay.on ? "#C9A45C" : "#D3DCE6"
                    c.fillRect(60, 130, 90, 50)
                    c.strokeStyle = "#8A6D35"
                    for (let x = 66; x < 150; x += 10) { c.beginPath(); c.moveTo(x, 130); c.lineTo(x + 6, 180); c.stroke() }
                    c.fillStyle = "#14213D"
                    c.fillText(qsTr("Spule"), 76, 205)
                    // Anker: Hebel ueber der Spule, Drehpunkt links
                    c.strokeStyle = "#14213D"
                    c.lineWidth = 6
                    const tipY = relay.on ? 118 : 88
                    c.beginPath(); c.moveTo(40, 110); c.lineTo(270, tipY); c.stroke()
                    c.beginPath(); c.arc(40, 110, 6, 0, 2 * Math.PI); c.fill()
                    c.fillText(qsTr("Anker"), 150, relay.on ? 100 : 78)
                    // Kontakt rechts: Anker beruehrt ihn, wenn die Spule zieht
                    c.lineWidth = 3
                    c.fillStyle = "#C9A45C"
                    c.fillRect(262, 122, 22, 8)
                    c.fillStyle = "#14213D"
                    c.fillText(qsTr("Kontakt"), 250, 150)
                    // Lampenstromkreis
                    c.strokeStyle = relay.on ? "#0E7490" : "#5B6B7C"
                    c.beginPath(); c.moveTo(284, 126); c.lineTo(360, 126); c.lineTo(360, 70); c.stroke()
                    c.beginPath(); c.arc(360, 52, 18, 0, 2 * Math.PI)
                    c.fillStyle = relay.on ? "#FFD54A" : "#E6EDF3"; c.fill(); c.stroke()
                    c.beginPath(); c.moveTo(347, 39); c.lineTo(373, 65); c.moveTo(373, 39); c.lineTo(347, 65); c.stroke()
                    // Magnetfeld andeuten
                    if (relay.on) {
                        c.strokeStyle = "#06B6D4"; c.lineWidth = 2
                        for (let i = 0; i < 3; ++i) { c.beginPath(); c.arc(105, 128, 14 + 8 * i, Math.PI * 1.15, Math.PI * 1.85); c.stroke() }
                    }
                }
            }
            ZButton {
                Layout.alignment: Qt.AlignHCenter
                objectName: "historyRelay"
                primary: relay.on
                text: relay.on ? qsTr("Strom in der Spule: an") : qsTr("Strom in der Spule: aus")
                onClicked: relay.on = !relay.on
            }
            Caption { text: qsTr("Fließt Strom durch die Spule, wird sie zum Magneten und zieht den Anker an. Der Anker schließt den Kontakt – die Lampe leuchtet. Das dauert ein paar Tausendstel Sekunden.") }
        }
    }

    Component { // ENIAC: Roehre und Zahlen
        id: visEniac
        RowLayout {
            spacing: 24
            Canvas {
                Layout.preferredWidth: 120
                Layout.preferredHeight: 200
                onPaint: {
                    const c = getContext("2d")
                    c.reset()
                    c.lineWidth = 3
                    c.strokeStyle = "#5B6B7C"
                    c.fillStyle = "#EEF3F7"
                    c.beginPath(); c.moveTo(25, 170); c.lineTo(25, 50); c.arc(60, 50, 35, Math.PI, 0); c.lineTo(95, 170); c.closePath()
                    c.fill(); c.stroke()
                    c.strokeStyle = "#F59E0B"; c.lineWidth = 4 // Heizdraht gluehend
                    c.beginPath(); c.moveTo(50, 150); c.lineTo(50, 70); c.lineTo(70, 70); c.lineTo(70, 150); c.stroke()
                    c.fillStyle = "#14213D"
                    c.fillRect(20, 170, 80, 16)
                    for (let x = 30; x <= 90; x += 15) c.fillRect(x, 186, 4, 12)
                }
            }
            ColumnLayout {
                spacing: 8
                Repeater {
                    model: [[qsTr("17 468"), qsTr("Röhren")], [qsTr("27 Tonnen"), qsTr("schwer, so viel wie vier bis fünf Elefanten")],
                            [qsTr("150 Kilowatt"), qsTr("Strom, so viel wie viele Herdplatten zugleich")]]
                    delegate: RowLayout {
                        required property var modelData
                        spacing: 10
                        Text {
                            Layout.preferredWidth: 150
                            text: modelData[0]
                            color: ZTheme.navy
                            font.pixelSize: Math.round(22 * ZTheme.fontScale); font.bold: true
                        }
                        Text {
                            Layout.preferredWidth: 260
                            text: modelData[1]
                            wrapMode: Text.WordWrap
                            color: ZTheme.text
                            font.pixelSize: Math.round(15 * ZTheme.fontScale)
                        }
                    }
                }
            }
        }
    }

    Component { // Transistor: Schaltgeschwindigkeit, jedes Kaestchen zehnmal schneller
        id: visSpeed
        ColumnLayout {
            spacing: 10
            Text {
                text: qsTr("So oft können sie in einer Sekunde umschalten (ungefähr):")
                color: ZTheme.text
                font.pixelSize: Math.round(16 * ZTheme.fontScale); font.bold: true
            }
            Repeater {
                model: [[qsTr("Relais"), 2, qsTr("100-mal")], [qsTr("Röhre"), 6, qsTr("1 Million Mal")],
                        [qsTr("Transistor im Chip heute"), 11, qsTr("100 Milliarden Mal")]]
                delegate: RowLayout {
                    required property var modelData
                    spacing: 10
                    Text {
                        Layout.preferredWidth: 190
                        text: modelData[0]
                        wrapMode: Text.WordWrap
                        color: ZTheme.text
                        font.pixelSize: Math.round(15 * ZTheme.fontScale)
                    }
                    Row {
                        spacing: 3
                        Repeater {
                            model: parent.parent.modelData[1]
                            delegate: Rectangle { width: 26; height: 24; radius: 4; color: ZTheme.accent }
                        }
                    }
                    Text {
                        text: modelData[2]
                        color: ZTheme.navy
                        font.pixelSize: Math.round(15 * ZTheme.fontScale); font.bold: true
                    }
                }
            }
            Caption { text: qsTr("Jedes Kästchen bedeutet: zehnmal so oft.") }
        }
    }

    Component { // 7400: vier NAND in einem Gehaeuse (Draufsicht, Kerbe links)
        id: visChip
        ColumnLayout {
            spacing: 8
            Canvas {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 480
                Layout.preferredHeight: 270
                onPaint: {
                    const c = getContext("2d")
                    c.reset()
                    const x0 = 40, x1 = 440, y0 = 60, y1 = 210, pitch = (x1 - x0) / 7.5
                    const px = i => x0 + pitch * (i + 0.75) // Stift-Platz 0..6
                    c.fillStyle = "#1F2937"
                    c.fillRect(x0, y0, x1 - x0, y1 - y0)
                    c.fillStyle = "#FBF6EA"
                    c.beginPath(); c.arc(x0, (y0 + y1) / 2, 14, -Math.PI / 2, Math.PI / 2); c.fill() // Kerbe
                    c.font = "bold 13px sans-serif"
                    c.textAlign = "center"
                    for (let i = 0; i < 7; ++i) { // unten 1..7 von links, oben 14..8 von links
                        c.fillStyle = "#9AA7B4"
                        c.fillRect(px(i) - 7, y1, 14, 18)
                        c.fillRect(px(i) - 7, y0 - 18, 14, 18)
                        c.fillStyle = "#14213D"
                        c.fillText(String(i + 1), px(i), y1 + 34)
                        c.fillText(String(14 - i), px(i), y0 - 24)
                    }
                    c.fillText("GND", px(6), y1 + 50)
                    c.fillText("+5 V", px(0), y0 - 40)
                    c.strokeStyle = "#E8C27A"; c.lineWidth = 2
                    function nand(a, b, y, out, up) { // Eingaenge an Stift a, b; Ausgang an Stift out
                        const gx = (px(a) + px(b)) / 2 - 14, gy = up ? y0 + 22 : y1 - 62
                        c.strokeRect(gx, gy, 28, 40)
                        c.fillStyle = "#E8C27A"; c.fillText("&", gx + 14, gy + 25)
                        c.beginPath(); c.arc(gx + 14, up ? gy + 46 : gy - 6, 5, 0, 2 * Math.PI); c.stroke()
                        const pin = up ? y0 : y1, edge = up ? gy : gy + 40, outY = up ? gy + 51 : gy - 11
                        c.beginPath()
                        c.moveTo(px(a), pin); c.lineTo(px(a), up ? pin + 10 : pin - 10); c.lineTo(gx + 7, up ? pin + 10 : pin - 10); c.lineTo(gx + 7, edge)
                        c.moveTo(px(b), pin); c.lineTo(px(b), up ? pin + 10 : pin - 10); c.lineTo(gx + 21, up ? pin + 10 : pin - 10); c.lineTo(gx + 21, edge)
                        c.moveTo(gx + 14, outY); c.lineTo(gx + 14, (y0 + y1) / 2); c.lineTo(px(out), (y0 + y1) / 2 + (up ? -4 : 4)); c.lineTo(px(out), pin)
                        c.stroke()
                    }
                    nand(0, 1, y1, 2, false)  // 1, 2 -> 3
                    nand(3, 4, y1, 5, false)  // 4, 5 -> 6
                    nand(5, 4, y0, 6, true)   // 9, 10 -> 8 (oben: Platz 5 = 9, 4 = 10, 6 = 8)
                    nand(2, 1, y0, 3, true)   // 12, 13 -> 11 (Platz 2 = 12, 1 = 13, 3 = 11)
                    c.fillStyle = "#FFFFFF"; c.font = "bold 16px sans-serif"
                    c.textAlign = "left"
                    c.fillText("7400", x0 + 20, (y0 + y1) / 2 + 6)
                }
            }
            Caption { text: qsTr("Der 7400 von oben: 14 Beinchen, darin vier NAND-Gatter. Beinchen 14 bekommt +5 Volt, Beinchen 7 ist Masse (GND).") }
        }
    }

    Component { // XODER: Halbaddierer zum Ausprobieren
        id: visAdder
        ColumnLayout {
            id: adder
            spacing: 12
            property bool a: false
            property bool b: false
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 10
                Toggle01 { objectName: "historyAdderA"; name: "A"; on: adder.a; onClicked: adder.a = !adder.a }
                Toggle01 { objectName: "historyAdderB"; name: "B"; on: adder.b; onClicked: adder.b = !adder.b }
            }
            GridLayout {
                Layout.alignment: Qt.AlignHCenter
                columns: 4
                columnSpacing: 12
                rowSpacing: 8
                Text { text: "A, B  →"; color: ZTheme.muted; font.pixelSize: 16; font.family: ZTheme.mono }
                GateShape { width: 70; height: 56; Layout.preferredWidth: 70; Layout.preferredHeight: 56; symbol: "=1"; us: workbench.symbolStyle === "us"; stroke: ZTheme.navy; fontSize: 22 }
                Text { text: "→"; color: ZTheme.muted; font.pixelSize: 20 }
                Lamp { on: adder.a !== adder.b; label: qsTr("Stelle") }
                Text { text: "A, B  →"; color: ZTheme.muted; font.pixelSize: 16; font.family: ZTheme.mono }
                GateShape { width: 70; height: 56; Layout.preferredWidth: 70; Layout.preferredHeight: 56; symbol: "&"; us: workbench.symbolStyle === "us"; stroke: ZTheme.navy; fontSize: 22 }
                Text { text: "→"; color: ZTheme.muted; font.pixelSize: 20 }
                Lamp { on: adder.a && adder.b; label: qsTr("Übertrag") }
            }
            Text {
                objectName: "historyAdderSum"
                Layout.alignment: Qt.AlignHCenter
                text: (adder.a ? 1 : 0) + " + " + (adder.b ? 1 : 0) + "  =  " + (adder.a && adder.b ? "1" : "0") + (adder.a !== adder.b ? "1" : "0")
                      + "   (" + qsTr("dezimal %1").arg((adder.a ? 1 : 0) + (adder.b ? 1 : 0)) + ")"
                color: ZTheme.navy
                font.pixelSize: 26; font.bold: true; font.family: ZTheme.mono
            }
        }
    }

    Component { // CMOS: Geraete mit Batterie
        id: visCmos
        ColumnLayout {
            spacing: 14
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 24
                Repeater {
                    model: [["🧮", qsTr("Taschenrechner")], ["⌚", qsTr("Digitaluhr")], ["📱", qsTr("Handy")]]
                    delegate: Column {
                        required property var modelData
                        spacing: 4
                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData[0]; font.pixelSize: 56; font.family: "Segoe UI Emoji" }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: modelData[1]
                            color: ZTheme.text
                            font.pixelSize: Math.round(15 * ZTheme.fontScale)
                        }
                    }
                }
            }
            Caption { text: qsTr("Ein CMOS-Gatter ist wie eine Tür mit zwei Flügeln: Immer ist genau einer zu. Deshalb fließt im Ruhezustand fast kein Strom.") }
        }
    }

    Component { // FPGA: Zellen umprogrammieren
        id: visFpga
        ColumnLayout {
            spacing: 10
            readonly property var kinds: [["&", false], ["≥1", false], ["=1", false], ["&", true], ["1", true]]
            GridLayout {
                Layout.alignment: Qt.AlignHCenter
                columns: 4
                rowSpacing: 8
                columnSpacing: 8
                Repeater {
                    model: 8
                    delegate: Rectangle {
                        id: cell
                        required property int index
                        property int kind: index % 5
                        Layout.preferredWidth: 96; Layout.preferredHeight: 80
                        radius: 10
                        color: cellTap.pressed ? "#D9F3F8" : "#F3FAFC"
                        border.color: ZTheme.accent
                        border.width: 2
                        GateShape {
                            anchors.centerIn: parent
                            width: 64; height: 50
                            symbol: parent.parent.parent.kinds[cell.kind][0]
                            inverted: parent.parent.parent.kinds[cell.kind][1]
                            us: workbench.symbolStyle === "us"
                            stroke: ZTheme.navy
                            fontSize: 18
                        }
                        TapHandler { id: cellTap; onTapped: cell.kind = (cell.kind + 1) % 5 }
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Logikzelle %1 umprogrammieren").arg(index + 1)
                    }
                }
            }
            Caption { text: qsTr("Ein echter FPGA hat Tausende bis Millionen solcher Zellen.") }
        }
    }

    Component { // heute: Zahlen
        id: visToday
        ColumnLayout {
            spacing: 12
            Repeater {
                model: [["🔬", qsTr("viele Milliarden"), qsTr("Transistoren auf einem Handy-Chip")],
                        ["📏", qsTr("wenige Nanometer"), qsTr("klein sind ihre feinsten Teile – ein Nanometer ist ein Millionstel Millimeter")],
                        ["⚡", qsTr("Milliarden Mal"), qsTr("pro Sekunde kann ein Gatter umschalten")]]
                delegate: RowLayout {
                    required property var modelData
                    spacing: 14
                    Text { text: modelData[0]; font.pixelSize: 40; font.family: "Segoe UI Emoji" }
                    ColumnLayout {
                        spacing: 0
                        Text {
                            text: modelData[1]
                            color: ZTheme.navy
                            font.pixelSize: Math.round(22 * ZTheme.fontScale); font.bold: true
                        }
                        Text {
                            Layout.preferredWidth: 380
                            text: modelData[2]
                            wrapMode: Text.WordWrap
                            color: ZTheme.text
                            font.pixelSize: Math.round(15 * ZTheme.fontScale)
                        }
                    }
                }
            }
        }
    }

    Component { // Ueberblick als Tabelle
        id: visOverview
        GridLayout {
            columns: 4
            columnSpacing: 0
            rowSpacing: 0
            Repeater {
                model: [qsTr("Gatter"), qsTr("Idee aus der Mathematik"), qsTr("Zuerst gebaut mit"), qsTr("Wofür?"),
                        qsTr("NICHT"), qsTr("Verneinung (Aristoteles, Boole)"), qsTr("Relais-Öffner, Röhre"), qsTr("ein Signal umdrehen"),
                        qsTr("UND, ODER"), qsTr("Boolesche Algebra (1854)"), qsTr("Schaltern, Relais, Dioden"), qsTr("Bedingungen prüfen, Maschinen steuern"),
                        qsTr("NAND, NOR"), qsTr("Sheffer-Strich (1913), Peirce-Pfeil (um 1880)"), qsTr("Transistoren, Chips wie dem 7400"), qsTr("alles aus einem einzigen Baustein bauen"),
                        qsTr("XODER"), qsTr("Rechnen mit Rest 2"), qsTr("mehreren Gattern zusammen"), qsTr("addieren, Fehler erkennen, verschlüsseln")]
                delegate: Rectangle {
                    required property string modelData
                    required property int index
                    readonly property bool head: index < 4
                    Layout.fillWidth: true
                    Layout.preferredWidth: index % 4 === 0 ? 110 : 220
                    Layout.preferredHeight: Math.max(48, cellText.implicitHeight + 16)
                    color: head ? ZTheme.navy : (Math.floor(index / 4) % 2 ? ZTheme.surface : "#F3FAFC")
                    border.color: ZTheme.line
                    Text {
                        id: cellText
                        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 8 }
                        text: parent.modelData
                        wrapMode: Text.WordWrap
                        color: parent.head ? "white" : ZTheme.text
                        font.pixelSize: Math.round(14 * ZTheme.fontScale)
                        font.bold: parent.head || parent.index % 4 === 0
                    }
                }
            }
        }
    }

    readonly property var visuals: ({ aristoteles: visAristoteles, leibniz: visLeibniz, boole: visBoole, peirce: visPeirce,
                                      shannon: visShannon, zuse: visRelay, eniac: visEniac, transistor: visSpeed,
                                      chip: visChip, xoder: visAdder, cmos: visCmos, fpga: visFpga, heute: visToday,
                                      ueberblick: visOverview })

    // ---------------------------------------------------------------- Seite
    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: column.implicitHeight + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: column
            x: Math.max(24, (flick.width - width) / 2)
            y: 24
            width: Math.min(flick.width - 48, 1100)
            spacing: 16

            RowLayout {
                Layout.fillWidth: true
                spacing: 14
                ZButton { objectName: "historyBack"; text: "← " + qsTr("Zurück"); onClicked: root.backRequested() }
                Text {
                    Layout.fillWidth: true
                    text: "🏛 " + qsTr("Historische Entwicklung")
                    color: ZTheme.navy
                    font.pixelSize: Math.round(26 * ZTheme.fontScale); font.bold: true
                }
                Text {
                    text: qsTr("Station %1 von %2").arg(root.current + 1).arg(root.stations.length)
                    color: ZTheme.muted
                    font.pixelSize: Math.round(14 * ZTheme.fontScale)
                }
            }

            // Zeitleiste: Jahr und Titel, die aktuelle Station mit dickem Rahmen (nicht nur Farbe)
            ListView {
                id: timeline
                objectName: "historyTimeline"
                Layout.fillWidth: true
                Layout.preferredHeight: 74
                orientation: ListView.Horizontal
                spacing: 6
                clip: true
                model: root.stations
                currentIndex: root.current
                boundsBehavior: Flickable.StopAtBounds
                onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)
                Component.onCompleted: positionViewAtIndex(currentIndex, ListView.Contain)
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    readonly property bool isCurrent: index === root.current
                    width: 140; height: 70
                    radius: 10
                    color: isCurrent ? ZTheme.navy : stopTap.pressed ? "#D9F3F8" : ZTheme.surface
                    border.width: isCurrent ? 3 : 1
                    border.color: isCurrent ? ZTheme.pad : ZTheme.line
                    Column {
                        anchors.centerIn: parent
                        width: parent.width - 12
                        spacing: 2
                        Text {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            text: parent.parent.modelData.year !== "" ? parent.parent.modelData.year : "★"
                            color: parent.parent.isCurrent ? "white" : ZTheme.navy
                            font.pixelSize: Math.round(15 * ZTheme.fontScale); font.bold: true
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            text: parent.parent.modelData.title
                            color: parent.parent.isCurrent ? "#E8C27A" : ZTheme.muted
                            font.pixelSize: Math.round(12 * ZTheme.fontScale)
                            elide: Text.ElideRight
                        }
                    }
                    TapHandler { id: stopTap; onTapped: root.go(parent.index) }
                    Accessible.role: Accessible.Button
                    Accessible.name: modelData.year + " " + modelData.title + (isCurrent ? ", " + qsTr("ausgewählt") : "")
                }
            }

            // Station: Zetti erzaehlt, daneben die Zeichnung
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: stationRow.implicitHeight + 40
                radius: 16
                color: ZTheme.surface
                border.color: "#D8CDB4"
                GridLayout {
                    id: stationRow
                    x: 20; y: 20
                    width: parent.width - 40
                    columns: width > 404 + (visual.item ? visual.item.implicitWidth : 520) ? 2 : 1
                    columnSpacing: 24
                    rowSpacing: 16
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 380
                        Layout.alignment: Qt.AlignTop
                        spacing: 10
                        Text {
                            Layout.fillWidth: true
                            text: (root.station.year !== "" ? root.station.year + " – " : "") + root.station.title
                            color: ZTheme.navy
                            font.pixelSize: Math.round(22 * ZTheme.fontScale); font.bold: true
                            wrapMode: Text.WordWrap
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Image {
                                Layout.preferredWidth: 90; Layout.preferredHeight: 112
                                Layout.alignment: Qt.AlignTop
                                source: "qrc:/zuse/zetti/zetti_" + root.station.pose + ".png"
                                fillMode: Image.PreserveAspectFit
                                mipmap: true
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: say.implicitHeight + 24
                                radius: 14
                                color: ZTheme.bubble
                                border.width: 2
                                border.color: ZTheme.accentDim
                                Text {
                                    id: say
                                    objectName: "historySay"
                                    anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
                                    wrapMode: Text.WordWrap
                                    color: ZTheme.text
                                    font.pixelSize: Math.round(16 * ZTheme.fontScale)
                                    lineHeight: 1.15
                                    text: root.station.text
                                }
                            }
                        }
                        Text {
                            visible: root.station.where !== ""
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: "🔧 " + root.station.where
                            color: ZTheme.accentText
                            font.pixelSize: Math.round(15 * ZTheme.fontScale); font.bold: true
                        }
                    }
                    Loader {
                        id: visual
                        Layout.fillWidth: true
                        Layout.preferredWidth: item ? item.implicitWidth : 520
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                        sourceComponent: root.visuals[root.station.key]
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                ZButton {
                    objectName: "historyPrev"
                    enabled: root.current > 0
                    text: "← " + qsTr("Vorherige Station")
                    onClicked: root.go(root.current - 1)
                }
                Item { Layout.fillWidth: true }
                ZButton {
                    objectName: "historyNext"
                    primary: true
                    visible: root.current < root.stations.length - 1
                    text: qsTr("Nächste Station") + " →"
                    onClicked: root.go(root.current + 1)
                }
                ZButton {
                    visible: root.current === root.stations.length - 1
                    primary: true
                    text: qsTr("Zum Lernpfad")
                    onClicked: root.backRequested()
                }
            }
        }
    }
}
