// Gezogene Leitung als weiche Kurve zwischen zwei Anschlusspunkten (ADR-0004).
import QtQuick
import QtQuick.Shapes

Shape {
    id: root
    property point from
    property point to
    property color wireColor: "red"
    property bool selected: false
    property real currentmA: 0 // Strom von 'from' nach 'to' (technische Stromrichtung)
    property real phase: 0

    // Punkt auf der Kurve (kubische Bezierkurve wie unten), t = 0..1
    function pointAt(t) {
        const c1x = from.x, c1y = from.y - 60, c2x = to.x, c2y = to.y - 60
        const u = 1 - t
        return Qt.point(u * u * u * from.x + 3 * u * u * t * c1x + 3 * u * t * t * c2x + t * t * t * to.x,
                        u * u * u * from.y + 3 * u * u * t * c1y + 3 * u * t * t * c2y + t * t * t * to.y)
    }
    // Geschwindigkeit der Stromflusspunkte ~ Stromstaerke (logarithmisch, damit uA und A sichtbar bleiben)
    readonly property real flowSpeed: {
        const i = Math.abs(currentmA)
        if (i < 0.05) return 0
        return Math.min(1.6, 0.15 + Math.log10(1 + i) * 0.45) * (currentmA > 0 ? 1 : -1)
    }
    readonly property int dotCount: Math.max(2, Math.round(Math.hypot(to.x - from.x, to.y - from.y) / 40))

    anchors.fill: parent
    preferredRendererType: Shape.CurveRenderer

    ShapePath {
        strokeColor: root.selected ? Theme.signalOrange : Qt.darker(root.wireColor, 1.4)
        strokeWidth: root.selected ? 10 : 7
        fillColor: "transparent"
        capStyle: ShapePath.RoundCap
        startX: root.from.x; startY: root.from.y
        PathCubic {
            x: root.to.x; y: root.to.y
            control1X: root.from.x; control1Y: root.from.y - 60
            control2X: root.to.x; control2Y: root.to.y - 60
        }
    }
    ShapePath {
        strokeColor: root.wireColor
        strokeWidth: 4
        fillColor: "transparent"
        capStyle: ShapePath.RoundCap
        startX: root.from.x; startY: root.from.y
        PathCubic {
            x: root.to.x; y: root.to.y
            control1X: root.from.x; control1Y: root.from.y - 60
            control2X: root.to.x; control2Y: root.to.y - 60
        }
    }

    // Stromfluss: wandernde Punkte (docs/ui-spec.md); stehen still ohne Strom
    FrameAnimation {
        running: root.visible && root.flowSpeed !== 0
        onTriggered: root.phase = (root.phase + root.flowSpeed * frameTime + 1) % 1
    }
    Repeater {
        model: root.flowSpeed !== 0 ? root.dotCount : 0
        Rectangle {
            required property int index
            readonly property point p: root.pointAt((root.phase + index / root.dotCount) % 1)
            width: 7; height: 7; radius: 3.5
            x: p.x - width / 2
            y: p.y - height / 2
            color: "#FFF6A8"
            border.color: "#8A6D00"
            border.width: 1
        }
    }
}
