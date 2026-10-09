// Schaltzeichen nach DIN EN 60617 (IEC 60617) fuer Bauteilfach und spaeter die Schaltplanansicht.
// Gezeichnet als SVG-Pfade (Qt Quick Shapes, direkt von der Grafikkarte, ohne Canvas) in einem Raster von
// 64 x 32 Einheiten (Anschluesse links und rechts), skaliert auf die Groesse des Elements. Einfarbig wie in der Norm.
import QtQuick
import QtQuick.Shapes

Item {
    id: sym
    property string type
    property color stroke: Theme.navy
    property real lineWidth: 2

    implicitWidth: 64
    implicitHeight: 32

    // Pfeil (Linie + gefuellte Spitze) von (x1,y1) nach (x2,y2)
    function arrowLine(x1, y1, x2, y2) { return "M" + x1 + " " + y1 + " L" + x2 + " " + y2 }
    function arrowHead(x1, y1, x2, y2) {
        const a = Math.atan2(y2 - y1, x2 - x1)
        const p = (d) => (x2 - 4.5 * Math.cos(a + d)).toFixed(2) + " " + (y2 - 4.5 * Math.sin(a + d)).toFixed(2)
        return "M" + x2 + " " + y2 + " L" + p(-0.5) + " L" + p(0.5) + " Z"
    }
    function circle(cx, cy, r) {
        return "M" + (cx - r) + " " + cy + " A" + r + " " + r + " 0 1 0 " + (cx + r) + " " + cy
             + " A" + r + " " + r + " 0 1 0 " + (cx - r) + " " + cy
    }
    readonly property string diode: "M0 16 H24 M38 16 H64 M24 8 L38 16 L24 24 Z M38 8 V24"

    // Strichpfade, gefuellte Pfade und Beschriftung je Bauteiltyp
    readonly property var shape: {
        switch (type) {
        case "widerstand":
            return {s: "M0 16 H18 M46 16 H64 M18 10 H46 V22 H18 Z", f: ""}
        case "fotowiderstand":
            return {s: "M0 16 H18 M46 16 H64 M18 12 H46 V22 H18 Z " + arrowLine(8, 0, 18, 9) + " " + arrowLine(16, 0, 26, 9),
                    f: arrowHead(8, 0, 18, 9) + " " + arrowHead(16, 0, 26, 9)}
        case "heissleiter": // Widerstand mit Kennlinien-Schraegstrich (nichtlinear, temperaturabhaengig) und -t
            return {s: "M0 16 H18 M46 16 H64 M18 10 H46 V22 H18 Z M12 28 L44 4 H50", f: "", t: "−ϑ", tx: 54, ty: 26}
        case "elektromagnet": // Spule (Wicklung als Boegen) mit Eisenkern (Strich darueber)
            return {s: "M0 16 H14 M50 16 H64 M14 16 A4.5 5 0 0 1 23 16 A4.5 5 0 0 1 32 16 A4.5 5 0 0 1 41 16 A4.5 5 0 0 1 50 16 M14 6 H50", f: ""}
        case "relais": // Antrieb (Rechteck) und Schliesser, Wirkverbindung gestrichelt
            return {s: "M0 16 H6 M22 16 H26 M6 9 H22 V23 H6 Z M26 16 H40 M40 16 L54 8 M54 16 H64 M14 9 V7 M14 5 V3 M14 3 H20 M24 3 H30 M34 3 H40 M44 3 H47 V10", f: ""}
        case "gluehlampe": // Kreis mit Kreuz
            return {s: "M0 16 H20 M44 16 H64 " + circle(32, 16, 12) + " M23.5 7.5 L40.5 24.5 M40.5 7.5 L23.5 24.5", f: ""}
        case "motor": // Kreis mit M
            return {s: "M0 16 H20 M44 16 H64 " + circle(32, 16, 12), f: "", t: "M", tx: 32, ty: 16}
        case "summer": // Halbkreis auf der Grundlinie
            return {s: "M0 16 H64 M20 16 A12 12 0 0 1 44 16", f: ""}
        case "taster": // Schliesser mit Druckknopf (Wirkverbindung gestrichelt)
            return {s: "M0 16 H22 M42 16 H64 M22 16 L44 8 M27 2 H37 M32 2 V4.5 M32 7 V9.5 M32 12 V13", f: ""}
        case "reedkontakt": // Schliesser im Schutzrohr
            return {s: "M0 16 H24 M40 16 H64 M24 16 L42 8 M18 16 A14 11 0 1 0 46 16 A14 11 0 1 0 18 16", f: ""}
        case "schalter":
            return {s: "M0 16 H22 M42 16 H64 M22 16 L44 8", f: ""}
        case "wechselschalter": // Mitte links, Stellungen rechts oben/unten
            return {s: "M0 16 H22 M42 8 H64 M42 24 H64 M22 16 L42 8", f: ""}
        case "diode_si":
            return {s: diode, f: ""}
        case "led_rot": case "led_gruen": case "led_gelb": case "led_blau": // Diode mit zwei Pfeilen nach aussen
            return {s: diode + " " + arrowLine(29, 7, 36, 0) + " " + arrowLine(35, 9, 42, 2),
                    f: arrowHead(29, 7, 36, 0) + " " + arrowHead(35, 9, 42, 2)}
        case "kondensator":
            return {s: "M0 16 H29 M35 16 H64 M29 6 V26 M35 6 V26", f: ""}
        case "elko": // gepolt: Plusbelag hohl, Minusbelag gefuellt
            return {s: "M0 16 H25 M39 16 H64 M25 6 H29 V26 H25 Z", f: "M35 6 H39 V26 H35 Z", t: "+", tx: 19, ty: 7}
        case "transistor_npn": // Basis links, Kollektor oben, Emitter unten mit Pfeil nach aussen
            return {s: circle(34, 16, 14) + " M8 16 H28 M28 8 V24 M28 12 L40 4 V0 " + arrowLine(28, 20, 39, 27) + " M39 27 L40 32",
                    f: arrowHead(28, 20, 39, 27)}
        case "batterie": // langer duenner Strich = Pluspol, kurzer dicker = Minuspol
            return {s: "M0 16 H28 M36 16 H64 M28 4 V28", f: "M34 10 H38 V22 H34 Z", t: "+", tx: 22, ty: 6}
        case "lautsprecher": // Magnet (Rechteck) mit beiden Spulenanschluessen, Trichter seitlich (DIN EN 60617)
            return {s: "M0 16 H24 M40 16 H64 M24 12 H40 V20 H24 Z M26 12 L20 2 H44 L38 12", f: ""}
        case "pultschalter":
            return {s: "M0 16 H22 M42 16 H64 M22 16 L44 8", f: ""}
        case "potentiometer": // Widerstand mit Schleifer (Pfeil von unten)
            return {s: "M0 16 H18 M46 16 H64 M18 10 H46 V22 H18 Z " + arrowLine(32, 32, 32, 23), f: arrowHead(32, 32, 32, 23)}
        case "messgeraet":
            return {s: "M0 16 H20 M44 16 H64 " + circle(32, 16, 12), f: "", t: "V", tx: 32, ty: 16}
        case "blinkmodul": case "tonmodul":
            return {s: "M0 16 H14 M50 16 H64 M14 6 H50 V26 H14 Z", f: "", t: type === "blinkmodul" ? "BL" : "TON", tx: 32, ty: 16}
        default:
            return {s: "M0 16 H20 M44 16 H64 M20 8 H44 V24 H20 Z", f: ""}
        }
    }

    Item {
        id: grid
        width: 64
        height: 32
        readonly property real k: Math.min(sym.width / 64, sym.height / 32)
        x: (sym.width - 64 * k) / 2
        y: (sym.height - 32 * k) / 2
        transformOrigin: Item.TopLeft
        scale: k

        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeColor: sym.stroke
                strokeWidth: sym.lineWidth / grid.k
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                PathSvg { path: sym.shape.s }
            }
            ShapePath {
                strokeColor: "transparent"
                fillColor: sym.stroke
                PathSvg { path: sym.shape.f || "M0 0" }
            }
        }
        Text {
            visible: sym.shape.t !== undefined
            text: sym.shape.t || ""
            x: (sym.shape.tx || 0) - width / 2
            y: (sym.shape.ty || 0) - height / 2
            color: sym.stroke
            font.pixelSize: sym.shape.t === "+" ? 10 : 13
            font.bold: true
        }
    }
}
