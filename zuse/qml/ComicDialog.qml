import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Erklaer-Comic zu den Grundfunktionen mit Bildern aus dem Kinderalltag (Vorgabe des Inhabers, 25.10.2026):
// UND = Tuersteher-Regel, ODER = Eisdielen-Regel, NICHT = Gegenteil-Tag, NAND = verbotene Kombination,
// NOR = strenge Ruhe-Regel, XODER = Entscheidungsspiel. Jedes Bild: Szene (Bedingungen -> Ergebnis, mit Schalter- und
// Lampenbild) und Zetti mit Sprechblase; die Sprache macht WENN, DANN, UND, ODER, NICHT sichtbar. Bei UND, ODER und
// NICHT ein Bild »Zum Anfassen«: Stromkreis mit Schaltern in Reihe, parallel bzw. als Oeffner, zum Antippen.
// Angeboten nach dem Entdecken (verraet sonst die Raetselkiste), beim Bauen (XODER) und in der Werkzeugkiste.
Popup {
    id: root
    modal: true
    focus: true
    padding: 20
    width: Math.min(parent ? parent.width - 60 : 1000, 1080)
    height: Math.min(parent ? parent.height - 40 : 760, 820)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? (parent.height - height) / 2 : 0
    background: Rectangle { radius: 20; color: "#FFFDF7"; border.color: ZTheme.navy; border.width: 2 }

    property string key: "und"
    function show(k) {
        key = k
        open()
    }

    function scene(a, b, outText, outOn) {
        return { ins: b === undefined ? [a] : [a, b], out: { t: outText, on: outOn } }
    }
    function cond(tOn, tOff, on) { return { t: on ? tOn : tOff, on: on } }

    readonly property var comics: ({
        "und": {
            title: qsTr("UND – Die Türsteher-Regel"), symbol: "&", inverted: false,
            intro: qsTr("»Du darfst ins Kino, wenn du deine Hausaufgaben gemacht hast und dein Zimmer aufgeräumt ist.«"),
            panels: [
                { pose: "gruebelnd", scene: scene(cond(qsTr("Hausaufgaben gemacht"), qsTr("Hausaufgaben nicht gemacht"), false), cond(qsTr("Zimmer aufgeräumt"), qsTr("Zimmer nicht aufgeräumt"), false), qsTr("Kein Kino"), false),
                  say: qsTr("Nichts erledigt? Der Türsteher lässt dich nicht rein.") },
                { pose: "nachdenklich", scene: scene(cond(qsTr("Hausaufgaben gemacht"), qsTr("Hausaufgaben nicht gemacht"), true), cond(qsTr("Zimmer aufgeräumt"), qsTr("Zimmer nicht aufgeräumt"), false), qsTr("Kein Kino"), false),
                  say: qsTr("Hausaufgaben fertig, aber das Zimmer ist ein Chaos – die Tür bleibt zu.") },
                { pose: "ueberrascht", scene: scene(cond(qsTr("Hausaufgaben gemacht"), qsTr("Hausaufgaben nicht gemacht"), false), cond(qsTr("Zimmer aufgeräumt"), qsTr("Zimmer nicht aufgeräumt"), true), qsTr("Kein Kino"), false),
                  say: qsTr("Zimmer ordentlich, aber die Hausaufgaben fehlen – immer noch zu!") },
                { pose: "begeistert", scene: scene(cond(qsTr("Hausaufgaben gemacht"), qsTr("Hausaufgaben nicht gemacht"), true), cond(qsTr("Zimmer aufgeräumt"), qsTr("Zimmer nicht aufgeräumt"), true), qsTr("Ab ins Kino!"), true),
                  say: qsTr("<b>WENN</b> die Hausaufgaben gemacht sind <b>UND</b> das Zimmer aufgeräumt ist, <b>DANN</b> geht es ins Kino!") },
                { pose: "erklaerend", circuit: "reihe",
                  say: qsTr("Zum Anfassen: Zwei Schalter hintereinander (in Reihe). Der Strom muss durch beide – die Lampe leuchtet nur, wenn A <b>UND</b> B an sind.") }
            ],
            merke: qsTr("UND: Beide Bedingungen müssen erfüllt sein. Fehlt auch nur eine, bleibt die Tür zu.")
        },
        "oder": {
            title: qsTr("ODER – Die Eisdielen-Regel"), symbol: "≥1", inverted: false,
            intro: qsTr("»Du bekommst zum Nachtisch ein Eis oder einen Apfel.«"),
            panels: [
                { pose: "nachdenklich", scene: scene(cond(qsTr("Eis da"), qsTr("kein Eis"), false), cond(qsTr("Apfel da"), qsTr("kein Apfel"), false), qsTr("Kein Nachtisch"), false),
                  say: qsTr("Weder Eis noch Apfel? Dann gibt es keinen Nachtisch – schade!") },
                { pose: "freundlich", scene: scene(cond(qsTr("Eis da"), qsTr("kein Eis"), true), cond(qsTr("Apfel da"), qsTr("kein Apfel"), false), qsTr("Nachtisch!"), true),
                  say: qsTr("Ein Eis – der Nachtisch ist gerettet!") },
                { pose: "zeigend", scene: scene(cond(qsTr("Eis da"), qsTr("kein Eis"), false), cond(qsTr("Apfel da"), qsTr("kein Apfel"), true), qsTr("Nachtisch!"), true),
                  say: qsTr("Ein Apfel – auch prima!") },
                { pose: "begeistert", scene: scene(cond(qsTr("Eis da"), qsTr("kein Eis"), true), cond(qsTr("Apfel da"), qsTr("kein Apfel"), true), qsTr("Nachtisch!"), true),
                  say: qsTr("Beides? Umso besser! <b>WENN</b> es Eis <b>ODER</b> einen Apfel gibt, <b>DANN</b> gibt es Nachtisch – beides ist auch erlaubt.") },
                { pose: "erklaerend", circuit: "parallel",
                  say: qsTr("Zum Anfassen: Zwei Schalter nebeneinander (parallel). Der Strom hat zwei Wege – die Lampe leuchtet, wenn A <b>ODER</b> B an ist, oder beide.") }
            ],
            merke: qsTr("ODER: Es reicht, wenn mindestens eine Bedingung stimmt – beide dürfen es auch.")
        },
        "nicht": {
            title: qsTr("NICHT – Der Gegenteil-Tag"), symbol: "1", inverted: true,
            intro: qsTr("Heute ist Gegenteil-Tag! Zettis Zauberschild sagt immer das Gegenteil."),
            panels: [
                { pose: "freundlich", scene: scene(cond(qsTr("Sonne scheint"), qsTr("Sonne scheint nicht"), true), undefined, qsTr("Schild: »Regen!«"), false),
                  say: qsTr("<b>WENN</b> die Sonne scheint, <b>DANN</b> sagt das Schild: <b>NICHT</b>-Sonne – also Regen!") },
                { pose: "ueberrascht", scene: scene(cond(qsTr("Sonne scheint"), qsTr("Sonne scheint nicht"), false), undefined, qsTr("Schild: »Sonne!«"), true),
                  say: qsTr("Es regnet? Dann behauptet das Schild: Sonne!") },
                { pose: "erklaerend",
                  say: qsTr("<b>NICHT</b> kehrt den Zustand einfach um: Aus Ja wird Nein, aus An wird Aus – und umgekehrt.") },
                { pose: "zeigend", circuit: "oeffner",
                  say: qsTr("Zum Anfassen: Ein Taster als Öffner. Die Lampe ist an – drückst du, geht sie aus. Drücken heißt hier: <b>NICHT</b> leuchten.") }
            ],
            merke: qsTr("NICHT dreht um: Aus An wird Aus, aus Aus wird An.")
        },
        "nand": {
            title: qsTr("NAND – Die verbotene Kombination"), symbol: "&", inverted: true,
            intro: qsTr("»Du darfst alles machen, außer gleichzeitig rennen und die volle Suppenschüssel tragen.«"),
            panels: [
                { pose: "freundlich", scene: scene(cond(qsTr("Rennen"), qsTr("nicht rennen"), false), cond(qsTr("Suppe tragen"), qsTr("keine Suppe tragen"), false), qsTr("Erlaubt"), true),
                  say: qsTr("Ruhig stehen, keine Schüssel – erlaubt.") },
                { pose: "zeigend", scene: scene(cond(qsTr("Rennen"), qsTr("nicht rennen"), true), cond(qsTr("Suppe tragen"), qsTr("keine Suppe tragen"), false), qsTr("Erlaubt"), true),
                  say: qsTr("Rennen ohne Schüssel – erlaubt!") },
                { pose: "nachdenklich", scene: scene(cond(qsTr("Rennen"), qsTr("nicht rennen"), false), cond(qsTr("Suppe tragen"), qsTr("keine Suppe tragen"), true), qsTr("Erlaubt"), true),
                  say: qsTr("Die Schüssel langsam tragen – auch erlaubt.") },
                { pose: "ueberrascht", scene: scene(cond(qsTr("Rennen"), qsTr("nicht rennen"), true), cond(qsTr("Suppe tragen"), qsTr("keine Suppe tragen"), true), qsTr("Verboten!"), false),
                  say: qsTr("Rennen <b>UND</b> Suppe tragen – das ist die verbotene Kombination! Nur hier heißt es Nein.") },
                { pose: "erklaerend",
                  say: qsTr("Das ist ein <b>UND</b> mit einem <b>NICHT</b> dahinter: <b>NICHT-UND</b>, kurz NAND. Solange <b>NICHT</b> beides zugleich passiert, heißt es Ja. So arbeitet auch die Fensterwarnung.") }
            ],
            merke: qsTr("NAND: Fast alles ist erlaubt – nur beides gleichzeitig nicht.")
        },
        "nor": {
            title: qsTr("NOR – Die strenge Ruhe-Regel"), symbol: "≥1", inverted: true,
            intro: qsTr("»Es gibt heute weder Fernseher noch Tablet.«"),
            panels: [
                { pose: "freundlich", scene: scene(cond(qsTr("Fernseher an"), qsTr("Fernseher aus"), false), cond(qsTr("Tablet an"), qsTr("Tablet aus"), false), qsTr("Ruhe"), true),
                  say: qsTr("Weder Fernseher noch Tablet – herrliche Ruhe!") },
                { pose: "ueberrascht", scene: scene(cond(qsTr("Fernseher an"), qsTr("Fernseher aus"), true), cond(qsTr("Tablet an"), qsTr("Tablet aus"), false), qsTr("Keine Ruhe"), false),
                  say: qsTr("Der Fernseher läuft – vorbei mit der Ruhe.") },
                { pose: "nachdenklich", scene: scene(cond(qsTr("Fernseher an"), qsTr("Fernseher aus"), false), cond(qsTr("Tablet an"), qsTr("Tablet aus"), true), qsTr("Keine Ruhe"), false),
                  say: qsTr("Das Tablet ist an – auch keine Ruhe.") },
                { pose: "gruebelnd", scene: scene(cond(qsTr("Fernseher an"), qsTr("Fernseher aus"), true), cond(qsTr("Tablet an"), qsTr("Tablet aus"), true), qsTr("Keine Ruhe"), false),
                  say: qsTr("Beides an? Erst recht nicht!") },
                { pose: "erklaerend",
                  say: qsTr("<b>WENN</b> der Fernseher <b>ODER</b> das Tablet an ist, <b>DANN</b> ist <b>NICHT</b> Ruhe. Das ist <b>NICHT-ODER</b>, kurz NOR.") }
            ],
            merke: qsTr("NOR: Nur wenn weder das eine noch das andere passiert, herrscht Ruhe.")
        },
        "xoder": {
            title: qsTr("XODER – Das Entscheidungsspiel"), symbol: "=1", inverted: false,
            intro: qsTr("»Du kannst jetzt entweder draußen Fahrrad fahren oder drinnen Lego bauen – aber nicht beides gleichzeitig.«"),
            panels: [
                { pose: "nachdenklich", scene: scene(cond(qsTr("Fahrrad fahren"), qsTr("nicht Fahrrad fahren"), false), cond(qsTr("Lego bauen"), qsTr("nicht Lego bauen"), false), qsTr("Nichts passiert"), false),
                  say: qsTr("Keins von beiden? Dann passiert gar nichts.") },
                { pose: "begeistert", scene: scene(cond(qsTr("Fahrrad fahren"), qsTr("nicht Fahrrad fahren"), true), cond(qsTr("Lego bauen"), qsTr("nicht Lego bauen"), false), qsTr("Das geht!"), true),
                  say: qsTr("Fahrrad fahren – super!") },
                { pose: "froehlich", scene: scene(cond(qsTr("Fahrrad fahren"), qsTr("nicht Fahrrad fahren"), false), cond(qsTr("Lego bauen"), qsTr("nicht Lego bauen"), true), qsTr("Das geht!"), true),
                  say: qsTr("Lego bauen – auch super!") },
                { pose: "ueberrascht", scene: scene(cond(qsTr("Fahrrad fahren"), qsTr("nicht Fahrrad fahren"), true), cond(qsTr("Lego bauen"), qsTr("nicht Lego bauen"), true), qsTr("Geht nicht!"), false),
                  say: qsTr("Draußen radeln und drinnen bauen zugleich? Das geht nicht!") },
                { pose: "erklaerend",
                  say: qsTr("Genau eines muss stimmen: das eine <b>ODER</b> das andere – aber <b>NICHT</b> beide. Entweder-oder: XODER. Das Treppenlicht funktioniert genauso.") }
            ],
            merke: qsTr("XODER: Genau eines von beiden – nicht keins und nicht beides.")
        }
    })
    readonly property var comic: comics[key] || comics["und"]

    component LampIcon: Rectangle {
        property bool on: false
        implicitWidth: 26; implicitHeight: 26
        radius: 13
        color: on ? "#FDE68A" : ZTheme.surfaceHigh
        border.width: 2
        border.color: on ? "#D97706" : ZTheme.zero
        Text { anchors.centerIn: parent; visible: parent.on; text: "✦"; color: "#B45309"; font.pixelSize: 14 }
    }
    component SwitchIcon: Rectangle {
        property bool on: false
        implicitWidth: 34; implicitHeight: 20
        radius: 10
        color: on ? ZTheme.accent : ZTheme.surfaceHigh
        border.color: on ? ZTheme.accentText : ZTheme.zero
        Rectangle {
            width: 14; height: 14; radius: 7
            anchors.verticalCenter: parent.verticalCenter
            x: parent.on ? parent.width - width - 3 : 3
            color: ZTheme.surface
            border.color: ZTheme.zero
        }
    }

    // Stromkreis zum Antippen: Batterie, Schalter (in Reihe, parallel oder ein Taster als Oeffner), Lampe.
    // Physik ehrlich: Die Lampe leuchtet genau dann, wenn ein geschlossener Weg von Plus nach Minus besteht.
    component Circuit: ColumnLayout {
        id: circuit
        property string kind: "reihe"
        property bool a: false
        property bool b: false
        readonly property bool closedA: kind === "oeffner" ? !a : a // Oeffner: gedrueckt = Kontakt offen
        readonly property bool lit: kind === "reihe" ? (a && b) : kind === "parallel" ? (a || b) : closedA
        spacing: 6
        onLitChanged: canvas.requestPaint()
        onAChanged: canvas.requestPaint()
        onBChanged: canvas.requestPaint()

        Canvas {
            id: canvas
            Layout.fillWidth: true
            Layout.preferredHeight: 170
            onWidthChanged: requestPaint()
            onPaint: {
                const c = getContext("2d")
                c.reset()
                const w = width, h = height
                const L = 30, R = w - 30, T = 46, B = h - 20
                const wire = circuit.lit ? "#0E7490" : "#64748B"
                c.lineWidth = 3
                c.strokeStyle = wire
                c.lineCap = "round"
                function line(x1, y1, x2, y2) { c.beginPath(); c.moveTo(x1, y1); c.lineTo(x2, y2); c.stroke() }
                function contact(x, y) { c.beginPath(); c.arc(x, y, 4, 0, 2 * Math.PI); c.fillStyle = wire; c.fill() }
                // Schalter zwischen x1 und x2 auf Hoehe y; geschlossen: gerade, offen: Hebel schraeg nach oben
                function sw(x1, x2, y, closed, label) {
                    contact(x1, y); contact(x2, y)
                    c.lineWidth = 4
                    if (closed) line(x1, y, x2, y)
                    else line(x1, y, x2 - 6, y - 22)
                    c.lineWidth = 3
                    c.fillStyle = "#14213D"
                    c.font = "bold 15px sans-serif"
                    c.fillText(label, (x1 + x2) / 2 - 5, y - 26)
                }
                const bx = L, by = (T + B) / 2
                // Batterie links (langer Strich Plus, kurzer Minus)
                line(L, T, bx, by - 12); line(bx, by + 12, L, B)
                c.lineWidth = 4; line(bx - 16, by - 8, bx + 16, by - 8); c.lineWidth = 6; line(bx - 8, by + 8, bx + 8, by + 8)
                c.lineWidth = 3
                c.fillStyle = "#14213D"; c.font = "bold 13px sans-serif"
                c.fillText("+", bx + 20, by - 4); c.fillText("−", bx + 14, by + 14)
                // Lampe rechts: Kreis mit Kreuz (DIN), leuchtend gelb gefuellt
                const lx = R, ly = (T + B) / 2, r = 16
                line(R, T, lx, ly - r); line(lx, ly + r, R, B)
                c.beginPath(); c.arc(lx, ly, r, 0, 2 * Math.PI)
                c.fillStyle = circuit.lit ? "#FDE68A" : "#F1F5F9"; c.fill(); c.stroke()
                const d = r * 0.7
                line(lx - d, ly - d, lx + d, ly + d); line(lx + d, ly - d, lx - d, ly + d)
                if (circuit.lit) { // Strahlen
                    c.strokeStyle = "#D97706"
                    for (let k = 0; k < 8; ++k) {
                        const ang = k * Math.PI / 4
                        line(lx + Math.cos(ang) * (r + 4), ly + Math.sin(ang) * (r + 4), lx + Math.cos(ang) * (r + 11), ly + Math.sin(ang) * (r + 11))
                    }
                    c.strokeStyle = wire
                }
                // untere Leitung
                line(L, B, R, B)
                // obere Leitung mit Schaltern
                const x1 = L + (R - L) * 0.28, x2 = x1 + 44, x3 = L + (R - L) * 0.58, x4 = x3 + 44
                if (circuit.kind === "reihe") {
                    line(L, T, x1, T); sw(x1, x2, T, circuit.a, "A"); line(x2, T, x3, T); sw(x3, x4, T, circuit.b, "B"); line(x4, T, R, T)
                } else if (circuit.kind === "parallel") {
                    const y2 = T + 52, p = L + (R - L) * 0.2, q = L + (R - L) * 0.75
                    const m1 = (p + q) / 2 - 22, m2 = m1 + 44
                    line(L, T, p, T); line(q, T, R, T)
                    line(p, T, m1, T); sw(m1, m2, T, circuit.a, "A"); line(m2, T, q, T)
                    line(p, T, p, y2); line(q, T, q, y2)
                    line(p, y2, m1, y2); sw(m1, m2, y2, circuit.b, "B"); line(m2, y2, q, y2)
                    contact(p, T); contact(q, T)
                } else { // Oeffner: Taster, im Ruhezustand geschlossen
                    const m1 = L + (R - L) * 0.42, m2 = m1 + 44
                    line(L, T, m1, T); line(m2, T, R, T)
                    contact(m1, T); contact(m2, T)
                    c.lineWidth = 4
                    if (circuit.closedA) line(m1, T + 6, m2, T + 6) // Bruecke liegt an
                    else line(m1, T - 14, m2, T - 14)               // gedrueckt: Bruecke abgehoben
                    line((m1 + m2) / 2, T + (circuit.closedA ? 6 : -14), (m1 + m2) / 2, T - 28) // Stoessel
                    c.lineWidth = 3
                    c.fillStyle = "#14213D"; c.font = "bold 15px sans-serif"
                    c.fillText("A", m2 + 8, T - 18)
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            ZButton {
                text: circuit.kind === "oeffner" ? (circuit.a ? qsTr("A loslassen") : qsTr("A drücken"))
                                                 : (circuit.a ? qsTr("A ausschalten") : qsTr("A einschalten"))
                onClicked: circuit.a = !circuit.a
            }
            ZButton {
                visible: circuit.kind !== "oeffner"
                text: circuit.b ? qsTr("B ausschalten") : qsTr("B einschalten")
                onClicked: circuit.b = !circuit.b
            }
            Text {
                Layout.fillWidth: true
                text: circuit.lit ? "✦ " + qsTr("Lampe leuchtet") : qsTr("Lampe aus")
                color: circuit.lit ? "#B45309" : ZTheme.muted
                font.pixelSize: Math.round(16 * ZTheme.fontScale); font.bold: true
                horizontalAlignment: Text.AlignRight
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            GateShape {
                Layout.preferredWidth: 64; Layout.preferredHeight: 52
                symbol: root.comic.symbol
                inverted: root.comic.inverted
                us: workbench.symbolStyle === "us"
                stroke: ZTheme.navy
                fontSize: 22
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    Layout.fillWidth: true
                    text: root.comic.title
                    color: ZTheme.navy
                    font.pixelSize: Math.round(26 * ZTheme.fontScale); font.bold: true
                    wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    text: root.comic.intro
                    color: ZTheme.accentText
                    font.pixelSize: Math.round(17 * ZTheme.fontScale); font.italic: true
                    wrapMode: Text.WordWrap
                }
            }
            ZButton { objectName: "comicClose"; text: qsTr("Schließen"); onClicked: root.close() }
        }

        Flickable {
            id: flick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: grid.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            GridLayout {
                id: grid
                width: flick.width - 12
                columns: width > 640 ? 2 : 1
                columnSpacing: 14
                rowSpacing: 14
                Repeater {
                    model: root.opened ? root.comic.panels : [] // neu aufbauen: Stromkreis startet ausgeschaltet
                    delegate: Rectangle { // ein Comicbild
                        id: panel
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        Layout.preferredHeight: panelCol.implicitHeight + 24
                        Layout.alignment: Qt.AlignTop
                        radius: 6
                        color: ZTheme.surface
                        border.width: 3
                        border.color: ZTheme.navy
                        Rectangle { // Bildnummer
                            anchors { right: parent.right; top: parent.top; margins: 6 }
                            width: 24; height: 24; radius: 12
                            color: ZTheme.navy
                            Text { anchors.centerIn: parent; text: panel.index + 1; color: "white"; font.pixelSize: 13; font.bold: true }
                        }
                        ColumnLayout {
                            id: panelCol
                            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
                            spacing: 8
                            Rectangle { // Szene: Bedingungen -> Ergebnis
                                visible: !!panel.modelData.scene
                                Layout.fillWidth: true
                                Layout.rightMargin: 24
                                Layout.preferredHeight: sceneRow.implicitHeight + 16
                                radius: 8
                                color: "#EEF6FB"
                                RowLayout {
                                    id: sceneRow
                                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 10 }
                                    spacing: 10
                                    ColumnLayout {
                                        spacing: 6
                                        Repeater {
                                            model: panel.modelData.scene ? panel.modelData.scene.ins : []
                                            delegate: RowLayout {
                                                required property var modelData
                                                spacing: 6
                                                SwitchIcon { on: modelData.on }
                                                Text {
                                                    text: modelData.t
                                                    color: modelData.on ? ZTheme.accentText : ZTheme.muted
                                                    font.pixelSize: Math.round(14 * ZTheme.fontScale); font.bold: true
                                                }
                                            }
                                        }
                                    }
                                    Text { text: "→"; color: ZTheme.navy; font.pixelSize: 26; font.bold: true }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6
                                        LampIcon { on: panel.modelData.scene ? panel.modelData.scene.out.on : false }
                                        Text {
                                            Layout.fillWidth: true
                                            wrapMode: Text.WordWrap
                                            text: panel.modelData.scene ? panel.modelData.scene.out.t : ""
                                            color: ZTheme.text
                                            font.pixelSize: Math.round(15 * ZTheme.fontScale); font.bold: true
                                        }
                                    }
                                }
                            }
                            Circuit {
                                visible: !!panel.modelData.circuit
                                Layout.fillWidth: true
                                Layout.rightMargin: 24
                                kind: panel.modelData.circuit || "reihe"
                            }
                            RowLayout { // Zetti mit Sprechblase
                                Layout.fillWidth: true
                                spacing: 6
                                Image {
                                    Layout.preferredWidth: 70; Layout.preferredHeight: 88
                                    Layout.alignment: Qt.AlignBottom
                                    source: "qrc:/zuse/zetti/zetti_" + panel.modelData.pose + ".png"
                                    fillMode: Image.PreserveAspectFit
                                    mipmap: true
                                }
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: sayText.implicitHeight + 20
                                    Layout.alignment: Qt.AlignVCenter
                                    radius: 14
                                    color: ZTheme.bubble
                                    border.width: 2
                                    border.color: ZTheme.accentDim
                                    Text {
                                        id: sayText
                                        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 10 }
                                        wrapMode: Text.WordWrap
                                        textFormat: Text.StyledText
                                        text: panel.modelData.say
                                        color: ZTheme.text
                                        font.pixelSize: Math.round(16 * ZTheme.fontScale)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle { // Merksatz
            Layout.fillWidth: true
            Layout.preferredHeight: merkText.implicitHeight + 20
            radius: 12
            color: "#FFF7E6"
            border.color: ZTheme.pad
            Text {
                id: merkText
                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 12 }
                wrapMode: Text.WordWrap
                text: qsTr("Merke: ") + root.comic.merke
                color: ZTheme.text
                font.pixelSize: Math.round(17 * ZTheme.fontScale); font.bold: true
            }
        }
    }
}
