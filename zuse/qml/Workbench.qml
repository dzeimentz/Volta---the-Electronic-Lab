import QtQuick
import QtQuick.Shapes

// Arbeitsflaeche der Werkbank. Koordinaten in dp des Rahmens (workbench.frameWidth x frameHeight), auf die
// verfuegbare Flaeche skaliert. Leitung ziehen: von einem Loetpad zum anderen; Eingang antippen schaltet um;
// Baustein antippen waehlt aus, doppelt antippen oeffnet »Blick hinein«.
Item {
    id: bench
    property string selectedPart: ""
    property int selectedWire: -1
    readonly property bool editable: !workbench.inside
    readonly property real s: Math.min(width / workbench.frameWidth, height / workbench.frameHeight)

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
        x: (bench.width - width * bench.s) / 2
        y: (bench.height - height * bench.s) / 2

        Rectangle { // Platine mit Punktraster
            anchors.fill: parent
            radius: 14
            color: ZTheme.board
            border.width: 2
            border.color: workbench.inside ? ZTheme.pad : ZTheme.accentDim
            Canvas {
                anchors.fill: parent
                onPaint: {
                    const ctx = getContext("2d")
                    ctx.fillStyle = ZTheme.dot
                    for (let x = 28; x < width; x += 28)
                        for (let y = 28; y < height; y += 28) ctx.fillRect(x - 1, y - 1, 2, 2)
                }
            }
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
        Repeater {
            model: workbench.ports
            delegate: Item {
                id: port
                required property var modelData
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
                    enabled: bench.editable
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
        const id = workbench.addPart(type, p.x - 60, p.y - 56)
        if (id !== "") { selectedWire = -1; selectedPart = id }
        return id !== ""
    }
    // Antippen im Fach: in eine freie Gegend der Werkbank
    function placePart(type) {
        const n = workbench.parts.length
        const id = workbench.addPart(type, 260 + (n % 4) * 170, 80 + Math.floor(n / 4) * 150 + (n % 2) * 40)
        if (id !== "") { selectedWire = -1; selectedPart = id }
    }
}
