// Logikanalysator (ab Volta 3, Stufe 7) unter der Arbeitsflaeche: eine Zeile je gestecktem Logik-Tastkopf LA1-LA8
// mit dem Pegelverlauf im Zeitfenster (neueste Werte rechts). H liegt oben, L unten, der verbotene Bereich ? in der
// Mitte als gestreiftes Band - Lage, Buchstabe und Muster unterscheiden die Pegel, nicht die Farbe allein. Rechts
// steht der aktuelle Pegel als Buchstabe. Bedienung: Zeitfenster fein (20 ms, zeigt das Prellen) oder grob (5 s),
// Standbild, Trigger je Kanal (egal, H, L, steigende oder fallende Flanke; mehrere Kanaele zusammen = Muster),
// zwei Zeitmarken A und B (Antippen der Spuren, rasten an Flanken ein) mit dem Abstand in µs/ms und die
// Schwellen fuer H und L (in Schritten von 10 % der Batterie). Das Bild rechnet der LabController.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Rectangle {
    id: panel
    required property var controller
    color: "white"
    border.color: Theme.fieldOutline
    property var ls: ({channels: []})
    property bool frozen: false
    property bool showTrigger: false
    property bool showThresholds: false
    // Zeitmarken als Anteil 0 ... 1 im Fenster (-1: nicht gesetzt)
    property real markA: -1
    property real markB: -1
    readonly property var chans: ls.channels || []
    readonly property real rowH: 26
    readonly property real laneX: 44
    readonly property real laneW: rows.width - laneX - 30
    readonly property var conditionText: ["–", "H", "L", "↑", "↓"]

    function refresh() { panel.ls = panel.controller.logicState() }
    // Takt aus dem LabController (stateChanged, ca. 10 Hz) wie Messgeraet und Module: laeuft auch, wenn das Fenster
    // gerade nicht gezeichnet wird (ein QML-Timer haengt am Bildtakt und blieb dort stehen)
    onVisibleChanged: if (visible && !frozen) refresh()
    onFrozenChanged: if (visible && !frozen) refresh()
    Connections {
        target: panel.controller
        function onStateChanged() { if (panel.visible && !panel.frozen) panel.refresh() }
        function onDeskChanged() { if (panel.visible && !panel.frozen) panel.refresh() }
    }
    // Fensterlaenge in Sekunden aus dem Kopf (fein 20 ms, grob 5 s)
    readonly property real windowS: ls.fine === true ? 0.02 : 5.0
    function timeText(s) { return panel.controller.formulaFormat(s, "s") }
    // naechste Flanke (Abschnittsgrenze) in allen Kanaelen, wenn sie naeher als 12 Bildpunkte liegt
    function snap(x) {
        let best = x, dist = 12 / Math.max(1, panel.laneW)
        for (const ch of panel.chans) {
            const s = ch.segs || []
            for (let i = 3; i + 2 < s.length; i += 3) {
                const d = Math.abs(s[i] - x)
                if (d < dist) { dist = d; best = s[i] }
            }
        }
        return best
    }
    function placeMark(x) {
        const v = snap(Math.max(0, Math.min(1, x)))
        if (markA < 0 || markB >= 0) { markA = v; markB = -1 }
        else markB = v
    }

    implicitHeight: head.height + (showTrigger ? trigBar.height + 4 : 0) + (showThresholds ? thrBar.height + 4 : 0)
                    + Math.max(1, chans.length) * rowH + 14

    Flow {
        id: head
        x: 8; y: 4
        width: panel.width - 16
        spacing: 8
        Label {
            height: Theme.control
            verticalAlignment: Text.AlignVCenter
            text: qsTr("Logikanalysator")
            font.bold: true
            font.pixelSize: Math.round(14 * Theme.fontScale)
            color: Theme.navy
        }
        Button {
            objectName: "logicFine"
            height: Theme.control
            checkable: true
            checked: panel.ls.fine === true
            text: panel.ls.fine === true ? qsTr("fein: %1").arg(panel.ls.windowText || "") : qsTr("grob: %1").arg(panel.ls.windowText || "")
            onClicked: { panel.controller.setLogicFine(!(panel.ls.fine === true)); panel.markA = -1; panel.markB = -1 }
            Accessible.name: qsTr("Zeitfenster umschalten")
        }
        Button {
            objectName: "logicFreeze"
            height: Theme.control
            checkable: true
            checked: panel.frozen
            text: panel.frozen ? qsTr("Weiter") : qsTr("Standbild")
            onClicked: panel.frozen = !panel.frozen
        }
        Button {
            objectName: "logicTriggerButton"
            height: Theme.control
            checkable: true
            checked: panel.showTrigger
            text: panel.ls.triggerOn !== true ? qsTr("Trigger: aus")
                  : panel.ls.waiting === true ? qsTr("Trigger: wartet") : qsTr("Trigger: ausgelöst")
            onClicked: panel.showTrigger = !panel.showTrigger
            Accessible.name: qsTr("Trigger einstellen")
        }
        Button {
            objectName: "logicThresholdButton"
            height: Theme.control
            checkable: true
            checked: panel.showThresholds
            text: qsTr("H über %1 · L unter %2").arg(panel.ls.highText || "").arg(panel.ls.lowText || "")
            onClicked: panel.showThresholds = !panel.showThresholds
            Accessible.name: qsTr("Schwellen einstellen")
        }
        Label {
            objectName: "logicMarkText"
            height: Theme.control
            verticalAlignment: Text.AlignVCenter
            text: panel.markA < 0 ? qsTr("Tippe auf die Spuren: Marke A, dann B.")
                  : panel.markB < 0 ? qsTr("Marke A gesetzt. Tippe für Marke B.")
                  : (Math.abs(panel.markB - panel.markA) * panel.windowS > 0
                     ? qsTr("A–B: %1 · 1 ÷ Δt = %2").arg(panel.timeText(Math.abs(panel.markB - panel.markA) * panel.windowS))
                                                     .arg(panel.controller.formulaFormat(1 / (Math.abs(panel.markB - panel.markA) * panel.windowS), "Hz"))
                     : qsTr("A–B: 0 s"))
            font.pixelSize: Math.round(12 * Theme.fontScale)
            color: Theme.navy
        }
        Button {
            objectName: "logicMarkClear"
            visible: panel.markA >= 0
            height: Theme.control
            text: qsTr("Marken weg")
            onClicked: { panel.markA = -1; panel.markB = -1 }
        }
    }

    // Trigger: je Kanal eine Taste, die egal -> H -> L -> steigend -> fallend durchschaltet
    Flow {
        id: trigBar
        visible: panel.showTrigger
        x: 8
        anchors.top: head.bottom; anchors.topMargin: 4
        width: panel.width - 16
        spacing: 6
        Repeater {
            model: panel.chans
            Button {
                required property var modelData
                readonly property int ch: Number(String(modelData.label).slice(2)) - 1
                readonly property int cond: panel.ls.trigger ? panel.ls.trigger[ch] : 0
                objectName: "logicTrig" + modelData.label
                height: Theme.control
                text: modelData.label + ": " + panel.conditionText[cond]
                onClicked: panel.controller.setLogicTrigger(ch, (cond + 1) % 5)
                Accessible.name: qsTr("Trigger %1: %2").arg(modelData.label)
                                 .arg([qsTr("egal"), qsTr("H"), qsTr("L"), qsTr("steigende Flanke"), qsTr("fallende Flanke")][cond])
            }
        }
        Button {
            objectName: "logicTrigClear"
            height: Theme.control
            text: qsTr("Trigger aus")
            onClicked: panel.controller.clearLogicTrigger()
        }
        Label {
            width: Math.min(trigBar.width, 420)
            height: Theme.control
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            text: qsTr("↑ steigende, ↓ fallende Flanke. Mehrere Kanäle zusammen sind ein Muster. Der Auslöser steht bei T.")
            font.pixelSize: Math.round(12 * Theme.fontScale)
            color: "#4A5866"
        }
    }

    // Schwellen in Schritten von 10 % der Batteriespannung
    Flow {
        id: thrBar
        visible: panel.showThresholds
        x: 8
        anchors.top: panel.showTrigger ? trigBar.bottom : head.bottom; anchors.topMargin: 4
        width: panel.width - 16
        spacing: 6
        readonly property real hi: panel.ls.high !== undefined ? panel.ls.high : 0.7
        readonly property real lo: panel.ls.low !== undefined ? panel.ls.low : 0.3
        function set(h, l) { panel.controller.setLogicThresholds(Math.round(h * 10) / 10, Math.round(l * 10) / 10) }
        Label { height: Theme.control; verticalAlignment: Text.AlignVCenter; text: qsTr("H über %1").arg(panel.ls.highText || ""); color: Theme.navy; font.bold: true }
        Button { objectName: "logicHighDown"; height: Theme.control; width: Theme.control; text: "−"; enabled: thrBar.hi > thrBar.lo + 0.15; onClicked: thrBar.set(thrBar.hi - 0.1, thrBar.lo); Accessible.name: qsTr("H-Schwelle tiefer") }
        Button { objectName: "logicHighUp"; height: Theme.control; width: Theme.control; text: "+"; enabled: thrBar.hi < 0.85; onClicked: thrBar.set(thrBar.hi + 0.1, thrBar.lo); Accessible.name: qsTr("H-Schwelle höher") }
        Label { height: Theme.control; verticalAlignment: Text.AlignVCenter; text: qsTr("L unter %1").arg(panel.ls.lowText || ""); color: Theme.navy; font.bold: true }
        Button { objectName: "logicLowDown"; height: Theme.control; width: Theme.control; text: "−"; enabled: thrBar.lo > 0.15; onClicked: thrBar.set(thrBar.hi, thrBar.lo - 0.1); Accessible.name: qsTr("L-Schwelle tiefer") }
        Button { objectName: "logicLowUp"; height: Theme.control; width: Theme.control; text: "+"; enabled: thrBar.lo < thrBar.hi - 0.15; onClicked: thrBar.set(thrBar.hi, thrBar.lo + 0.1); Accessible.name: qsTr("L-Schwelle höher") }
        Button { objectName: "logicThrReset"; height: Theme.control; text: qsTr("Standard"); onClicked: panel.controller.setLogicThresholds(0.7, 0.3) }
        Label {
            width: Math.min(thrBar.width, 420)
            height: Theme.control
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            text: qsTr("Die Schwellen gelten nur für die Anzeige. Die Bausteine selbst schalten bei ihren eigenen Schwellen.")
            font.pixelSize: Math.round(12 * Theme.fontScale)
            color: "#4A5866"
        }
    }

    Column {
        id: rows
        x: 8
        anchors.top: panel.showThresholds ? thrBar.bottom : panel.showTrigger ? trigBar.bottom : head.bottom
        anchors.topMargin: 4
        width: panel.width - 16
        Repeater {
            model: panel.chans
            Item {
                id: row
                required property var modelData
                required property int index
                width: rows.width
                height: panel.rowH
                Label {
                    id: name
                    width: panel.laneX
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.modelData.label
                    font.bold: true
                    font.pixelSize: Math.round(12 * Theme.fontScale)
                    color: Theme.navy
                }
                Rectangle {
                    id: lane
                    x: name.width
                    width: panel.laneW
                    height: row.height - 4
                    y: 2
                    color: index % 2 ? "#F4F7FA" : "#EDF2F6"
                    clip: true
                    readonly property var segs: row.modelData.segs || []
                    // verbotener Bereich: gestreiftes Band in der Mitte
                    Repeater {
                        model: Math.floor(lane.segs.length / 3)
                        Rectangle {
                            required property int index
                            readonly property real x0: lane.segs[3 * index] * lane.width
                            readonly property real x1: lane.segs[3 * index + 1] * lane.width
                            visible: lane.segs[3 * index + 2] === 1
                            x: x0; width: Math.max(1, x1 - x0)
                            y: lane.height * 0.3; height: lane.height * 0.4
                            color: "#F2C230"
                            border.color: "#8A6A00"; border.width: 1
                        }
                    }
                    Shape {
                        anchors.fill: parent
                        preferredRendererType: Shape.CurveRenderer
                        ShapePath {
                            strokeColor: "#1F6F3A"
                            strokeWidth: 2
                            fillColor: "transparent"
                            joinStyle: ShapePath.MiterJoin
                            PathPolyline {
                                path: {
                                    const s = lane.segs, w = lane.width, h = lane.height, pts = []
                                    const yOf = lv => lv === 2 ? 2 : lv === 0 ? h - 2 : h / 2
                                    for (let i = 0; i + 2 < s.length; i += 3) {
                                        if (s[i + 2] === 1) continue // verbotener Bereich: nur das Band
                                        pts.push(Qt.point(s[i] * w, yOf(s[i + 2])))
                                        pts.push(Qt.point(s[i + 1] * w, yOf(s[i + 2])))
                                    }
                                    return pts.length > 1 ? pts : [Qt.point(0, h - 2), Qt.point(0, h - 2)]
                                }
                            }
                        }
                    }
                }
                Label {
                    x: lane.x + lane.width + 6
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.modelData.level === 2 ? "H" : row.modelData.level === 0 ? "L" : row.modelData.level === 1 ? "?" : "–"
                    font.bold: true
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    color: "#11161C"
                }
            }
        }
    }

    // Ausloeser T und Zeitmarken A, B ueber allen Spuren (gestrichelt, mit Buchstaben: nicht nur Farbe)
    Item {
        id: overlay
        x: rows.x + panel.laneX
        y: rows.y
        width: panel.laneW
        height: Math.max(1, panel.chans.length) * panel.rowH
        visible: panel.chans.length > 0
        component Mark: Item {
            id: mk
            property real pos: -1
            property string letter
            property color ink: Theme.navy
            visible: pos >= 0
            x: pos * overlay.width
            width: 1; height: overlay.height
            Column {
                Repeater {
                    model: Math.ceil(mk.height / 6)
                    Rectangle { width: 2; height: 3; color: mk.ink; y: 0 }
                }
                spacing: 3
            }
            Rectangle {
                x: -9; y: -2
                width: 18; height: 16; radius: 3
                color: mk.ink
                Label { anchors.centerIn: parent; text: mk.letter; color: "white"; font.bold: true; font.pixelSize: Math.round(11 * Theme.fontScale) }
            }
        }
        Mark { pos: panel.ls.triggerOn === true ? (panel.ls.triggerX || 0.2) : -1; letter: "T"; ink: "#8A3B00" }
        Mark { pos: panel.markA; letter: "A" }
        Mark { pos: panel.markB; letter: "B" }
        MouseArea {
            objectName: "logicMarkArea"
            anchors.fill: parent
            onClicked: (mouse) => panel.placeMark(mouse.x / overlay.width)
        }
    }

    Label {
        visible: panel.chans.length === 0
        x: 8; anchors.top: rows.top
        text: qsTr("Stecke einen Logik-Tastkopf auf ein Feld.")
        color: "#4A5866"
    }
}
