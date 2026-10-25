import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

// Schaltplan parallel zum Aufbau (wie bei Volta): automatisch angeordnet (workbench.schematic) – Eingaenge links,
// Bausteine nach dem Signalfluss, Lampen rechts; nur rechte Winkel, Punkte an Abzweigungen. Leitungen zeigen den
// Pegel in Farbe und zusaetzlich als Ziffer an den Anschluessen (Farbe nie allein).
Rectangle {
    id: root
    color: ZTheme.surface
    radius: ZTheme.radius
    border.color: ZTheme.line
    readonly property var plan: workbench.schematic
    readonly property bool us: workbench.symbolStyle === "us"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6
        Text {
            text: qsTr("Schaltplan")
            color: ZTheme.navy
            font.pixelSize: Math.round(18 * ZTheme.fontScale); font.bold: true
        }
        Text {
            visible: !root.plan.elements || root.plan.elements.length === 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Noch nichts aufgebaut.")
            color: ZTheme.muted
            font.pixelSize: Math.round(14 * ZTheme.fontScale)
        }
        Flickable { // grosse Plaene: nicht kleiner als 70 %, dafuer verschiebbar
            id: area
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            readonly property real pw: root.plan.width || 1
            readonly property real ph: root.plan.height || 1
            readonly property real s: Math.max(0.7, Math.min(width / pw, height / ph, 1.6))
            contentWidth: Math.max(width, pw * s)
            contentHeight: Math.max(height, ph * s)
            ScrollBar.horizontal: ScrollBar { policy: area.pw * area.s > area.width + 1 ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded }
            ScrollBar.vertical: ScrollBar {}

            Item {
                id: canvas
                width: area.pw; height: area.ph
                scale: area.s
                transformOrigin: Item.TopLeft
                x: Math.max(0, (area.width - width * area.s) / 2)
                y: Math.max(0, (area.height - height * area.s) / 2)

                Repeater { // Leitungen
                    model: root.plan.lines || []
                    delegate: Shape {
                        id: ln
                        required property var modelData
                        anchors.fill: parent
                        preferredRendererType: Shape.CurveRenderer
                        ShapePath {
                            strokeColor: ZTheme.levelColor(ln.modelData.level)
                            strokeWidth: ln.modelData.level === "1" ? 3 : 2
                            fillColor: "transparent"
                            joinStyle: ShapePath.MiterJoin
                            capStyle: ShapePath.SquareCap
                            PathPolyline {
                                path: {
                                    const p = ln.modelData.points, out = []
                                    for (let i = 0; i + 1 < p.length; i += 2) out.push(Qt.point(p[i], p[i + 1]))
                                    return out
                                }
                            }
                        }
                    }
                }
                Repeater { // Verbindungspunkte
                    model: root.plan.dots || []
                    delegate: Rectangle {
                        required property var modelData
                        x: modelData.x - 4; y: modelData.y - 4
                        width: 8; height: 8; radius: 4
                        color: ZTheme.levelColor(modelData.level)
                    }
                }
                Repeater { // Schaltzeichen, Eingaenge, Lampen
                    model: root.plan.elements || []
                    delegate: Item {
                        id: el
                        required property var modelData
                        readonly property bool isPart: modelData.kind === "part"
                        readonly property bool isIn: modelData.kind === "in"
                        x: isPart ? modelData.x : modelData.x - 8
                        y: isPart ? modelData.y : modelData.y - 8
                        width: isPart ? modelData.w : 16
                        height: isPart ? modelData.h : 16

                        GateShape {
                            visible: el.isPart
                            anchors.fill: parent
                            symbol: el.modelData.symbol || ""
                            inverted: !!el.modelData.inverted
                            us: root.us
                            stroke: ZTheme.navy
                            fill: el.modelData.own ? "#FFF7E6" : ZTheme.surface
                            fontSize: (el.modelData.symbol || "").length > 2 ? 13 : 18
                        }
                        Repeater { // Anschlussnamen der Erfindungen
                            model: el.isPart && el.modelData.own ? el.modelData.inPins.concat(el.modelData.outPins) : []
                            delegate: Text {
                                required property var modelData
                                readonly property bool atLeft: modelData.x <= el.modelData.x + 1
                                x: atLeft ? modelData.x - el.modelData.x + 3 : modelData.x - el.modelData.x - width - 3
                                y: modelData.y - el.modelData.y - height / 2
                                text: modelData.name
                                color: ZTheme.muted
                                font.pixelSize: Math.max(8, Math.round(10 / area.s))
                            }
                        }
                        // Eingang: Anschlusspunkt (offener Kreis), Lampe: Kreis mit Kreuz (DIN EN 60617-8)
                        Rectangle {
                            visible: !el.isPart
                            anchors.fill: parent
                            radius: width / 2
                            color: !el.isIn && el.modelData.level === "1" ? "#FDE68A" : ZTheme.surface
                            border.width: 2
                            border.color: ZTheme.levelColor(el.modelData.level)
                            Shape {
                                visible: !el.isIn
                                anchors.fill: parent
                                ShapePath {
                                    strokeColor: ZTheme.navy; strokeWidth: 1.5; fillColor: "transparent"
                                    startX: 3.5; startY: 3.5
                                    PathLine { x: 12.5; y: 12.5 }
                                    PathMove { x: 12.5; y: 3.5 }
                                    PathLine { x: 3.5; y: 12.5 }
                                }
                            }
                        }
                        Text { // Name am Anschluss mit Pegel; Schrift unabhaengig vom Massstab
                            visible: !el.isPart
                            anchors.verticalCenter: parent.verticalCenter
                            x: el.isIn ? -width - 3 : parent.width + 3
                            text: el.modelData.id + "=" + (el.modelData.level || "")
                            color: ZTheme.text
                            font.pixelSize: Math.round(12 * ZTheme.fontScale / area.s)
                            font.bold: true
                        }
                    }
                }
            }
        }
        Text { // Legende: Bedeutung der Schalter und Lampen
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: {
                const parts = []
                for (const e of root.plan.elements || [])
                    if (e.kind !== "part" && e.label) parts.push(e.id + " = " + e.label)
                return parts.join("   ")
            }
            visible: text !== ""
            color: ZTheme.muted
            font.pixelSize: Math.round(13 * ZTheme.fontScale)
        }
    }
}
