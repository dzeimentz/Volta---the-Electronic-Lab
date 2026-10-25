import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

// Arbeitsflaeche der Werkbank. Koordinaten in dp des Rahmens (workbench.frameWidth x frameHeight), auf die
// verfuegbare Flaeche skaliert. Leitung ziehen: von einem Loetpad zum anderen; Eingang antippen schaltet um;
// Baustein antippen waehlt aus, doppelt antippen oeffnet »Blick hinein«.
Item {
    id: bench
    property string selectedPart: ""
    property int selectedWire: -1
    readonly property bool editable: !workbench.inside && !workbench.benchLocked
    readonly property real sizerWidth: ZTheme.control + 8 // Feldgroesse +/- rechts neben dem Rahmen (wie Volta)
    readonly property real s: Math.min((width - sizerWidth) / workbench.frameWidth, height / workbench.frameHeight)

    // Leitung im Entstehen
    property string dragFrom: ""
    property point dragStart: Qt.point(0, 0)
    property point dragPos: Qt.point(0, 0)
    property string dragTarget: ""

    function clearSelection() { selectedPart = ""; selectedWire = -1 }

    function pins() {
        let list = []
        for (const p of workbench.ports) list.push({ endpoint: p.name, x: p.x, y: p.y })
        for (const part of workbench.parts) {
            for (const pin of part.inPins) list.push(pin)
            for (const pin of part.outPins) list.push(pin)
        }
        return list
    }
    function nearestPin(x, y, exclude) {
        let best = "", bestD = 40 * 40
        for (const p of pins()) {
            if (p.endpoint === exclude) continue
            const d = (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y)
            if (d < bestD) { bestD = d; best = p.endpoint }
        }
        return best
    }
    // Leitung von einem Anschluss aus ziehen (DragHandler ohne Ziel)
    function wireDrag(handler, endpoint, x, y) {
        if (!bench.editable) return
        const p = frame.mapFromItem(null, handler.centroid.scenePosition.x, handler.centroid.scenePosition.y)
        if (handler.active && dragFrom === "") {
            dragFrom = endpoint
            dragStart = Qt.point(x, y)
        }
        if (dragFrom === "") return
        dragPos = p
        dragTarget = nearestPin(p.x, p.y, dragFrom)
        if (!handler.active) {
            if (dragTarget !== "") workbench.connectPins(dragFrom, dragTarget)
            dragFrom = ""
            dragTarget = ""
        }
    }

    Item {
        id: frame
        width: workbench.frameWidth
        height: workbench.frameHeight
        scale: bench.s
        transformOrigin: Item.TopLeft
        x: (bench.width - bench.sizerWidth - width * bench.s) / 2
        y: (bench.height - height * bench.s) / 2

        LayerFrame { // Eingangsschicht | Logikschicht mit Raster | Ausgangsschicht
            anchors.fill: parent
            grid: workbench.grid
            logicLeft: workbench.logicLeft
            logicRight: workbench.logicRight
            borderColor: workbench.inside ? ZTheme.pad : ZTheme.accentDim
            TapHandler { onTapped: bench.clearSelection() }
        }

        // Leitungen: 1 durchgezogen in der Akzentfarbe, 0 grau gestrichelt, ? grau gepunktet
        Repeater {
            model: workbench.wires
            delegate: Item {
                id: wire
                required property var modelData
                anchors.fill: parent
                readonly property real dx: Math.max(40, Math.abs(modelData.x2 - modelData.x1) / 2)
                readonly property bool sel: bench.selectedWire === modelData.index
                Shape {
                    anchors.fill: parent
                    preferredRendererType: Shape.CurveRenderer
                    ShapePath {
                        strokeWidth: wire.sel ? 7 : wire.modelData.level === "1" ? 5 : 4
                        strokeColor: wire.sel ? ZTheme.text : ZTheme.levelColor(wire.modelData.level)
                        fillColor: "transparent"
                        capStyle: ShapePath.RoundCap
                        strokeStyle: wire.modelData.level === "1" || wire.sel ? ShapePath.SolidLine : ShapePath.DashLine
                        dashPattern: wire.modelData.level === "0" ? [3, 2] : [0.5, 2]
                        startX: wire.modelData.x1; startY: wire.modelData.y1
                        PathCubic {
                            x: wire.modelData.x2; y: wire.modelData.y2
                            control1X: wire.modelData.x1 + (wire.modelData.x2 >= wire.modelData.x1 ? wire.dx : -wire.dx)
                            control1Y: wire.modelData.y1
                            control2X: wire.modelData.x2 - (wire.modelData.x2 >= wire.modelData.x1 ? wire.dx : -wire.dx)
                            control2Y: wire.modelData.y2
                        }
                    }
                }
                Item { // Griff in der Mitte: Leitung auswaehlen
                    width: ZTheme.control; height: ZTheme.control
                    x: (wire.modelData.x1 + wire.modelData.x2) / 2 - width / 2
                    y: (wire.modelData.y1 + wire.modelData.y2) / 2 - height / 2
                    visible: bench.editable
                    Rectangle {
                        anchors.centerIn: parent
                        width: 12; height: 12; radius: 6
                        color: wire.sel ? ZTheme.text : ZTheme.surfaceHigh
                        border.color: ZTheme.levelColor(wire.modelData.level)
                        border.width: 2
                    }
                    TapHandler { onTapped: { bench.selectedPart = ""; bench.selectedWire = wire.modelData.index } }
                }
            }
        }

        // Anschluesse am Rahmen: Kippschalter und Taster links, LEDs rechts
        // Modell = Anzahl: die Elemente bleiben beim Umschalten bestehen (Taster behaelt sein Loslassen)
        Repeater {
            model: workbench.ports.length
            delegate: Item {
                id: port
                required property int index
                readonly property var modelData: workbench.ports[index] ?? ({})
                x: modelData.x - width / 2
                y: modelData.y - height / 2
                width: ZTheme.pin; height: ZTheme.pin
                PinPad {
                    anchors.fill: parent
                    endpoint: port.modelData.name
                    level: port.modelData.level
                    highlight: bench.dragTarget === port.modelData.name
                }
                PanelPort { // Kippschalter/Taster links neben dem Loetpad, LED rechts daneben
                    anchors.verticalCenter: parent.verticalCenter
                    x: port.modelData.input ? -width + 4 : port.width - 4
                    name: port.modelData.name
                    label: port.modelData.label || ""
                    kind: port.modelData.kind
                    level: port.modelData.level
                    stateText: port.modelData.state || ""
                    compact: !!port.modelData.compact
                    enabled: !workbench.inside // Schalter bedienen geht auch am fest verdrahteten Bedienfeld
                }
                DragHandler {
                    target: null
                    enabled: bench.editable
                    grabPermissions: PointerHandler.CanTakeOverFromAnything
                    onActiveChanged: bench.wireDrag(this, port.modelData.name, port.modelData.x, port.modelData.y)
                    onCentroidChanged: if (active) bench.wireDrag(this, port.modelData.name, port.modelData.x, port.modelData.y)
                }
            }
        }

        // Bausteine
        Repeater {
            model: workbench.parts
            delegate: Item {
                id: partItem
                required property var modelData
                x: modelData.x
                y: modelData.y
                width: modelData.w
                height: modelData.h
                GateBody {
                    anchors.fill: parent
                    part: partItem.modelData
                    selected: bench.selectedPart === partItem.modelData.id
                    DragHandler {
                        target: partItem
                        enabled: bench.editable
                        onActiveChanged: if (!active) workbench.movePart(partItem.modelData.id, partItem.x, partItem.y)
                    }
                    TapHandler {
                        onTapped: {
                            if (tapCount >= 2) {
                                if (workbench.lookInside(partItem.modelData.id)) bench.clearSelection()
                            } else if (bench.editable) {
                                bench.selectedWire = -1
                                bench.selectedPart = partItem.modelData.id
                            }
                        }
                        onLongPressed: if (bench.editable) { bench.selectedWire = -1; bench.selectedPart = partItem.modelData.id }
                    }
                }
                Repeater {
                    model: partItem.modelData.inPins.concat(partItem.modelData.outPins)
                    delegate: PinPad {
                        id: pad
                        required property var modelData
                        x: modelData.x - partItem.modelData.x - width / 2
                        y: modelData.y - partItem.modelData.y - height / 2
                        endpoint: modelData.endpoint
                        level: modelData.level
                        highlight: bench.dragTarget === modelData.endpoint || bench.dragFrom === modelData.endpoint
                        DragHandler {
                            target: null
                            enabled: bench.editable
                            grabPermissions: PointerHandler.CanTakeOverFromAnything
                            onActiveChanged: bench.wireDrag(this, pad.modelData.endpoint, pad.modelData.x, pad.modelData.y)
                            onCentroidChanged: if (active) bench.wireDrag(this, pad.modelData.endpoint, pad.modelData.x, pad.modelData.y)
                        }
                    }
                }
            }
        }

        Shape { // Leitung im Entstehen
            anchors.fill: parent
            visible: bench.dragFrom !== ""
            ShapePath {
                strokeWidth: 4
                strokeColor: ZTheme.text
                fillColor: "transparent"
                strokeStyle: ShapePath.DashLine
                dashPattern: [2, 2]
                startX: bench.dragStart.x; startY: bench.dragStart.y
                PathLine { x: bench.dragPos.x; y: bench.dragPos.y }
            }
        }

        Text { // Hinweis beim Blick hinein
            visible: workbench.inside
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 12 }
            text: qsTr("Blick hinein – nur ansehen. Die Werte kommen von außen.")
            color: ZTheme.pad
            font.family: ZTheme.mono
            font.pixelSize: 16
        }
    }

    // Ablage aus dem Bauteilfach: Szenenposition in Rahmenkoordinaten
    function dropPart(type, sceneX, sceneY) {
        const p = frame.mapFromItem(null, sceneX, sceneY)
        if (p.x < 0 || p.y < 0 || p.x > frame.width || p.y > frame.height) return false
        const id = workbench.addPart(type, p.x - 56, p.y - 56)
        if (id !== "") { selectedWire = -1; selectedPart = id }
        return id !== ""
    }
    // Antippen im Fach: in eine freie Gegend der Werkbank
    // Feldgroesse wie bei Volta: gelbe runde Knoepfe, dazwischen die Stufe; die kleinste ist die Vorgabe
    component SizeButton: AbstractButton {
        id: sizeButton
        property bool plus: true
        width: ZTheme.control; height: ZTheme.control
        ToolTip.visible: hovered
        Accessible.name: ToolTip.text
        contentItem: Item {
            Rectangle {
                anchors.centerIn: parent
                width: 34; height: 34; radius: 17
                color: !sizeButton.enabled ? "#C9CED4" : (sizeButton.down ? "#F2B705" : "#FFD23F")
                border.color: sizeButton.enabled ? "#111111" : "#9AA3AD"
                border.width: 2.5
                Rectangle { anchors.centerIn: parent; width: 16; height: 3; color: sizeButton.enabled ? "#111111" : "#9AA3AD" }
                Rectangle {
                    visible: sizeButton.plus
                    anchors.centerIn: parent; width: 3; height: 16
                    color: sizeButton.enabled ? "#111111" : "#9AA3AD"
                }
            }
        }
    }
    Column {
        objectName: "boardSizer"
        x: frame.x + frame.width * bench.s + 8
        y: frame.y
        visible: !workbench.inside
        SizeButton {
            objectName: "boardBigger"
            enabled: workbench.canGrowBoard
            onClicked: workbench.setBoardSize(workbench.boardSize + 1)
            ToolTip.text: qsTr("Größeres Feld: mehr Platz, kleinere Bausteine")
        }
        Text {
            width: ZTheme.control
            horizontalAlignment: Text.AlignHCenter
            text: (workbench.boardSize + 1) + "/3"
            color: ZTheme.navy
            font.pixelSize: 12; font.bold: true
            Accessible.name: qsTr("Feldgröße %1 von 3").arg(workbench.boardSize + 1)
        }
        SizeButton {
            objectName: "boardSmaller"
            plus: false
            enabled: workbench.canShrinkBoard
            onClicked: workbench.setBoardSize(workbench.boardSize - 1)
            ToolTip.text: workbench.canShrinkBoard || workbench.boardSize === 0
                          ? qsTr("Kleineres Feld: größere Bausteine")
                          : qsTr("Kleiner geht nicht: Bausteine liegen weiter außen. Schieb sie zuerst zusammen.")
        }
    }

    function placePart(type) {
        const n = workbench.parts.length
        const id = workbench.addPart(type, 224 + (n % 3) * 168, 84 + Math.floor(n / 3) * 168 + (n % 2) * 56)
        if (id !== "") { selectedWire = -1; selectedPart = id }
    }
}
