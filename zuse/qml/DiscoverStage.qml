import QtQuick
import QtQuick.Shapes

// Entdeckerstufe: Schalter links, Steckplatz mit der eingesetzten Raetselkiste in der Mitte, LED rechts; die Kisten
// liegen in der Bibliothek links. Das Kind probiert aus und findet die Regel (NICHT, UND, ODER) selbst. Koordinaten
// in dp eines Rahmens von 896 x 440 (Schichten wie die Werkbank, LayerFrame), auf die verfuegbare Flaeche skaliert.
Item {
    id: root
    readonly property real fw: 896
    readonly property real fh: 440
    readonly property real s: Math.min(width / fw, height / fh)

    readonly property var inputs: workbench.panelInputs
    readonly property var outputs: workbench.panelOutputs
    readonly property var boxList: workbench.boxes
    readonly property var current: workbench.boxIndex >= 0 ? boxList[workbench.boxIndex] : null

    // Steckplatz
    readonly property real sx: 368
    readonly property real sy: 100
    readonly property real sw: 160
    readonly property real sh: 200

    // Schalter und LEDs um die Hoehe des Steckplatzes verteilt (Mitte des Bedienelements 74 dp unter seiner Oberkante)
    function inY(i) { return sy + sh / 2 + (i - (inputs.length - 1) / 2) * 150 - 74 }
    function outY(i) { return sy + sh / 2 + (i - (outputs.length - 1) / 2) * 150 - 74 }

    Item {
        id: frame
        width: root.fw
        height: root.fh
        scale: root.s
        transformOrigin: Item.TopLeft
        x: (root.width - width * root.s) / 2
        y: (root.height - height * root.s) / 2

        LayerFrame { // Eingangsschicht | Logikschicht mit Steckplatz | Ausgangsschicht
            anchors.fill: parent
            grid: 28
            logicLeft: 168
            logicRight: root.fw - 168
        }

        // Leitungen: Schalter -> Kiste -> LED (1 durchgezogen, 0 gestrichelt, ohne Kiste gepunktet)
        Repeater {
            model: root.inputs.length
            delegate: Shape {
                required property int index
                readonly property var port: root.inputs[index]
                readonly property real y1: root.inY(index) + 74
                readonly property real y2: root.sy + root.sh * (index + 1) / (root.inputs.length + 1)
                anchors.fill: parent
                preferredRendererType: Shape.CurveRenderer
                ShapePath {
                    strokeWidth: port.level === "1" ? 6 : 4
                    strokeColor: ZTheme.levelColor(port.level)
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    strokeStyle: port.level === "1" ? ShapePath.SolidLine : ShapePath.DashLine
                    dashPattern: [3, 2]
                    startX: 160; startY: y1
                    PathCubic { x: root.sx; y: y2; control1X: 280; control1Y: y1; control2X: 280; control2Y: y2 }
                }
            }
        }
        Repeater {
            model: root.outputs.length
            delegate: Shape {
                required property int index
                readonly property var port: root.outputs[index]
                readonly property real y1: root.sy + root.sh * (index + 1) / (root.outputs.length + 1)
                readonly property real y2: root.outY(index) + 74
                anchors.fill: parent
                preferredRendererType: Shape.CurveRenderer
                ShapePath {
                    strokeWidth: port.level === "1" ? 6 : 4
                    strokeColor: ZTheme.levelColor(port.level)
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    strokeStyle: port.level === "1" ? ShapePath.SolidLine : ShapePath.DashLine
                    dashPattern: port.level === "0" ? [3, 2] : [0.5, 2]
                    startX: root.sx + root.sw; startY: y1
                    PathCubic { x: root.fw - 160; y: y2; control1X: 640; control1Y: y1; control2X: 640; control2Y: y2 }
                }
            }
        }

        // Schalter und Taster mit ihrer Bedeutung
        Repeater {
            model: root.inputs
            delegate: PanelPort {
                required property var modelData
                required property int index
                x: 36
                y: root.inY(index)
                name: modelData.name
                label: modelData.label
                kind: modelData.kind
                level: modelData.level
            }
        }
        // LEDs
        Repeater {
            model: root.outputs
            delegate: PanelPort {
                required property var modelData
                required property int index
                x: root.fw - 160
                y: root.outY(index)
                name: modelData.name
                label: modelData.label
                kind: "led"
                level: modelData.level
            }
        }

        // Steckplatz mit der eingesetzten Kiste
        Rectangle {
            x: root.sx; y: root.sy; width: root.sw; height: root.sh
            radius: 18
            color: root.current ? "#FFF7E6" : ZTheme.surfaceHigh
            border.width: 3
            border.color: root.current ? ZTheme.pad : ZTheme.line
            Text {
                visible: !root.current
                anchors.centerIn: parent
                width: parent.width - 24
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: qsTr("Setz eine Kiste ein")
                color: ZTheme.muted
                font.pixelSize: 16
            }
            Text {
                visible: !!root.current && !root.current.revealed
                anchors.centerIn: parent
                text: "?"
                color: ZTheme.padDark
                font.pixelSize: 96; font.bold: true
            }
            GateShape {
                visible: !!root.current && root.current.revealed
                anchors.centerIn: parent
                width: 120; height: 112
                symbol: root.current ? root.current.symbol : ""
                inverted: !!root.current && root.current.inverted
                us: workbench.symbolStyle === "us"
                stroke: ZTheme.navy
            }
            Text {
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.bottom; topMargin: 8 }
                visible: !!root.current
                text: root.current ? (root.current.revealed ? root.current.name
                                                             : qsTr("Kiste %1").arg(root.current.index + 1)) : ""
                color: ZTheme.text
                font.pixelSize: 18; font.bold: true
            }
        }
    }
}
