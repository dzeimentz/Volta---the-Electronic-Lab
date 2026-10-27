// Gezogene Leitung zwischen zwei Anschlusspunkten (ADR-0004): weicher Bogen, oder - wenn der Bogen ein Bauteil
// streifen wuerde - um die Bauteile herum gefuehrt (Wegpunkte vom Router, model/wire_router) mit runden Ecken.
import QtQuick
import QtQuick.Shapes

Shape {
    id: root
    property point from
    property point to
    property var waypoints: []  // Wegpunkte zwischen from und to (leer: Bogen)
    property color wireColor: "red"
    property bool selected: false
    property real currentmA: 0 // Strom von 'from' nach 'to' (technische Stromrichtung)
    property real phase: 0

    readonly property bool routed: waypoints.length > 0
    readonly property var polyline: [from].concat(waypoints).concat([to])
    readonly property var lengths: {
        const out = [0]
        for (let i = 1; i < polyline.length; ++i)
            out.push(out[i - 1] + Math.hypot(polyline[i].x - polyline[i - 1].x, polyline[i].y - polyline[i - 1].y))
        return out
    }
    // Pfad mit runden Ecken (Biegeradius hoechstens 16) als SVG
    readonly property string svgPath: {
        const p = polyline
        let d = "M " + p[0].x + " " + p[0].y
        for (let i = 1; i < p.length - 1; ++i) {
            const a = p[i - 1], c = p[i], b = p[i + 1]
            const la = Math.hypot(c.x - a.x, c.y - a.y), lb = Math.hypot(b.x - c.x, b.y - c.y)
            const r = Math.min(16, la / 2, lb / 2)
            const ax = c.x - (c.x - a.x) / Math.max(la, 1e-6) * r, ay = c.y - (c.y - a.y) / Math.max(la, 1e-6) * r
            const bx = c.x + (b.x - c.x) / Math.max(lb, 1e-6) * r, by = c.y + (b.y - c.y) / Math.max(lb, 1e-6) * r
            d += " L " + ax + " " + ay + " Q " + c.x + " " + c.y + " " + bx + " " + by
        }
        d += " L " + p[p.length - 1].x + " " + p[p.length - 1].y
        return d
    }

    // Punkt auf der Leitung, t = 0..1 (Bogen: kubische Bezierkurve wie unten; sonst entlang der Wegpunkte)
    function pointAt(t) {
        if (routed) {
            const total = lengths[lengths.length - 1]
            const s = t * total
            for (let i = 1; i < polyline.length; ++i) {
                if (s <= lengths[i] || i === polyline.length - 1) {
                    const seg = Math.max(lengths[i] - lengths[i - 1], 1e-6)
                    const u = Math.min(1, Math.max(0, (s - lengths[i - 1]) / seg))
                    return Qt.point(polyline[i - 1].x + (polyline[i].x - polyline[i - 1].x) * u,
                                    polyline[i - 1].y + (polyline[i].y - polyline[i - 1].y) * u)
                }
            }
        }
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
    readonly property real pathLength: routed ? lengths[lengths.length - 1] : Math.hypot(to.x - from.x, to.y - from.y)
    readonly property int dotCount: Math.max(2, Math.round(pathLength / 40))

    anchors.fill: parent
    preferredRendererType: Shape.CurveRenderer

    // Mantel und Ader: je ein Pfad fuer den Bogen und einer fuer die gefuehrte Leitung
    ShapePath {
        strokeColor: root.routed ? "transparent" : (root.selected ? Theme.signalOrange : Qt.darker(root.wireColor, 1.4))
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
        strokeColor: root.routed ? "transparent" : root.wireColor
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
    ShapePath {
        strokeColor: root.routed ? (root.selected ? Theme.signalOrange : Qt.darker(root.wireColor, 1.4)) : "transparent"
        strokeWidth: root.selected ? 10 : 7
        fillColor: "transparent"
        capStyle: ShapePath.RoundCap
        joinStyle: ShapePath.RoundJoin
        PathSvg { path: root.routed ? root.svgPath : "M 0 0" }
    }
    ShapePath {
        strokeColor: root.routed ? root.wireColor : "transparent"
        strokeWidth: 4
        fillColor: "transparent"
        capStyle: ShapePath.RoundCap
        joinStyle: ShapePath.RoundJoin
        PathSvg { path: root.routed ? root.svgPath : "M 0 0" }
    }

    // Stromfluss: wandernde Punkte (docs/ui-spec.md); stehen still ohne Strom
    FrameAnimation {
        running: root.visible && root.flowSpeed !== 0 && Theme.showCurrentFlow && !Theme.reduceMotion
        onTriggered: root.phase = (root.phase + root.flowSpeed * frameTime + 1) % 1
    }
    Repeater {
        model: root.flowSpeed !== 0 && Theme.showCurrentFlow ? root.dotCount : 0
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
