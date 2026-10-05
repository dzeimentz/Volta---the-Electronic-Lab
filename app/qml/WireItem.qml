// Gezogene Leitung als weiche Kurve zwischen zwei Anschlusspunkten (ADR-0004).
import QtQuick
import QtQuick.Shapes

Shape {
    id: root
    property point from
    property point to
    property color wireColor: "red"
    property bool selected: false

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
}
