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
        case "diode_si": case "diode_ge":
            return {s: diode, f: ""}
        case "zdiode_4v7": case "zdiode_5v6": // Kathodenstrich mit Abwinkelung (Z-Diode)
            return {s: "M0 16 H24 M38 16 H64 M24 8 L38 16 L24 24 Z M34 6 L38 8 V24 L42 26", f: ""}
        case "spule_100mh": case "spule_220uh": case "spule_1mh": case "spule_10mh": // Wicklung als Boegen
            return {s: "M0 16 H14 M50 16 H64 M14 16 A4.5 5 0 0 1 23 16 A4.5 5 0 0 1 32 16 A4.5 5 0 0 1 41 16 A4.5 5 0 0 1 50 16", f: ""}
        case "drossel_10mh": // Wicklung mit Ferritkern (gestrichelt darueber)
            return {s: "M0 16 H14 M50 16 H64 M14 16 A4.5 5 0 0 1 23 16 A4.5 5 0 0 1 32 16 A4.5 5 0 0 1 41 16 A4.5 5 0 0 1 50 16 M14 6 H20 M24 6 H30 M34 6 H40 M44 6 H50", f: ""}
        case "spule_1h": // Wicklung mit Eisenkern (Strich darueber)
            return {s: "M0 16 H14 M50 16 H64 M14 16 A4.5 5 0 0 1 23 16 A4.5 5 0 0 1 32 16 A4.5 5 0 0 1 41 16 A4.5 5 0 0 1 50 16 M14 6 H50", f: ""}
        case "led_rot": case "led_gruen": case "led_gelb": case "led_blau": case "led_weiss": // Diode mit zwei Pfeilen nach aussen
            return {s: diode + " " + arrowLine(29, 7, 36, 0) + " " + arrowLine(35, 9, 42, 2),
                    f: arrowHead(29, 7, 36, 0) + " " + arrowHead(35, 9, 42, 2)}
        case "ir_led": // LED-Zeichen mit »IR«
            return {s: diode + " " + arrowLine(29, 7, 36, 0) + " " + arrowLine(35, 9, 42, 2),
                    f: arrowHead(29, 7, 36, 0) + " " + arrowHead(35, 9, 42, 2), t: "IR", tx: 53, ty: 26}
        case "fototransistor": // ohne Basisanschluss, Kollektor links, Emitter rechts, Licht faellt ein
            return {s: circle(32, 19, 13) + " M0 16 H14 L26 23 M24 25 H40 M36 23 L50 16 H64 "
                       + arrowLine(18, 0, 26, 12) + " " + arrowLine(27, 0, 35, 12),
                    f: arrowHead(18, 0, 26, 12) + " " + arrowHead(27, 0, 35, 12) + " " + arrowHead(36, 23, 46, 18)}
        case "uebertrager": // zwei Wicklungen gegeneinander, Kern als Doppelstrich, Anfaenge mit Punkt
            return {s: "M6 2 H58 V30 H6 Z M14 11 A4.5 4 0 0 1 23 11 A4.5 4 0 0 1 32 11 A4.5 4 0 0 1 41 11 A4.5 4 0 0 1 50 11 "
                       + "M14 21 A4.5 4 0 0 0 23 21 A4.5 4 0 0 0 32 21 A4.5 4 0 0 0 41 21 A4.5 4 0 0 0 50 21 M14 15 H50 M14 17 H50",
                    f: "M9 6 A2 2 0 1 1 9 10 A2 2 0 1 1 9 6 M9 22 A2 2 0 1 1 9 26 A2 2 0 1 1 9 22"}
        case "optokoppler": // IR-LED und Fototransistor in einem Gehaeuse
            return {s: "M6 2 H58 V30 H6 Z M18 2 V10 M13 10 H23 L18 18 Z M13 18 H23 M18 18 V30 "
                       + arrowLine(25, 12, 33, 12) + " " + arrowLine(25, 19, 33, 19)
                       + " M38 9 V23 M38 12 L46 6 V2 M38 20 L46 26 V30",
                    f: arrowHead(25, 12, 33, 12) + " " + arrowHead(25, 19, 33, 19) + " " + arrowHead(38, 20, 46, 26)}
        case "spannungsregler_5v": // Kasten mit Ein links, Aus rechts, Masse unten
            return {s: "M0 16 H16 M48 16 H64 M16 5 H48 V25 H16 Z M32 25 V32", f: "", t: "5V", tx: 32, ty: 15}
        case "kondensator":
            return {s: "M0 16 H29 M35 16 H64 M29 6 V26 M35 6 V26", f: ""}
        case "drehkondensator": // Kondensator mit Stellpfeil (DIN EN 60617)
            return {s: "M0 16 H29 M35 16 H64 M29 6 V26 M35 6 V26 " + arrowLine(22, 28, 42, 4), f: arrowHead(22, 28, 42, 4)}
        case "elko": // gepolt: Plusbelag hohl, Minusbelag gefuellt
            return {s: "M0 16 H25 M39 16 H64 M25 6 H29 V26 H25 Z", f: "M35 6 H39 V26 H35 Z", t: "+", tx: 19, ty: 7}
        case "transistor_npn": // Basis links, Kollektor oben, Emitter unten mit Pfeil nach aussen
            return {s: circle(34, 16, 14) + " M8 16 H28 M28 8 V24 M28 12 L40 4 V0 " + arrowLine(28, 20, 39, 27) + " M39 27 L40 32",
                    f: arrowHead(28, 20, 39, 27)}
        case "transistor_pnp": // wie NPN, Emitterpfeil zeigt zur Basis hin
            return {s: circle(34, 16, 14) + " M8 16 H28 M28 8 V24 M28 12 L40 4 V0 " + arrowLine(39, 27, 28, 20) + " M39 27 L40 32",
                    f: arrowHead(39, 27, 29, 20.6)}
        case "batterie": // langer duenner Strich = Pluspol, kurzer dicker = Minuspol
            return {s: "M0 16 H28 M36 16 H64 M28 4 V28", f: "M34 10 H38 V22 H34 Z", t: "+", tx: 22, ty: 6}
        case "lautsprecher": // Magnet (Rechteck) mit beiden Spulenanschluessen, Trichter seitlich (DIN EN 60617)
            return {s: "M0 16 H24 M40 16 H64 M24 12 H40 V20 H24 Z M26 12 L20 2 H44 L38 12", f: ""}
        case "pultschalter":
            return {s: "M0 16 H22 M42 16 H64 M22 16 L44 8", f: ""}
        case "potentiometer": // Widerstand mit Schleifer (Pfeil von unten)
            return {s: "M0 16 H18 M46 16 H64 M18 10 H46 V22 H18 Z " + arrowLine(32, 32, 32, 23), f: arrowHead(32, 32, 32, 23)}
        case "trimmer_10k": case "trimmer_100k": // Widerstand mit Einstellstrich (Schraegstrich mit Querstrich)
            return {s: "M0 16 H18 M46 16 H64 M18 10 H46 V22 H18 Z M20 28 L44 4 M40 0 L48 8", f: ""}
        case "messgeraet": case "messstelle_v": // Kreis mit Messgroesse (DIN EN 60617)
            return {s: "M0 16 H20 M44 16 H64 " + circle(32, 16, 12), f: "", t: "V", tx: 32, ty: 16}
        case "messstelle_a":
            return {s: "M0 16 H20 M44 16 H64 " + circle(32, 16, 12), f: "", t: "A", tx: 32, ty: 16}
        case "wechselquelle": // Wechselspannungsquelle: Kreis mit Sinus (DIN EN 60617)
            return {s: "M0 16 H20 M44 16 H64 " + circle(32, 16, 12) + " M25 16 Q28.5 8 32 16 Q35.5 24 39 16", f: ""}
        case "generator": // Funktionsgenerator: Kreis mit G und Sinus
            return {s: "M0 16 H20 M44 16 H64 " + circle(32, 16, 12), f: "", t: "G~", tx: 32, ty: 16}
        case "kennlinienschreiber": // Kennlinienschreiber: Kasten mit Kennlinie
            return {s: "M0 16 H18 M46 16 H64 M18 4 H46 V28 H18 Z M22 24 H30 Q36 24 42 8", f: "", t: "+", tx: 12, ty: 6}
        case "antenne": // Antenne der Senderanlage (DIN EN 60617): Mast mit Dreieck, Bezug 0V (ab Volta 3)
            return {s: "M0 16 H30 V8 M22 0 L30 8 L38 0 M30 8 V0", f: "", t: "ANT", tx: 50, ty: 16}
        case "tastkopf": // Tastkopf: Messspitze mit Kanalkreis, Bezug 0V
            return {s: "M0 16 H22 " + circle(34, 16, 12) + " M34 28 V31 M28 31 H40", f: "", t: "K", tx: 34, ty: 16}
        case "messstelle_ohm":
            return {s: "M0 16 H20 M44 16 H64 " + circle(32, 16, 12), f: "", t: "Ω", tx: 32, ty: 16}
        case "opmodul": // Dreieck mit IN+ oben, IN- unten (DIN EN 60617 vereinfacht)
            return {s: "M0 8 H16 M0 24 H16 M48 16 H64 M16 2 L48 16 L16 30 Z M19 8 H25 M22 5 V11 M19 24 H25", f: ""}
        case "timer555": // Kasten mit Aufschrift
            return {s: "M0 16 H14 M50 16 H64 M14 2 H50 V30 H14 Z", f: "", t: "555", tx: 32, ty: 16}
        case "blinkmodul": case "tonmodul": case "zeitmodul": case "verstaerkermodul": case "mwmodul":
        case "ukwmodul":
            return {s: "M0 16 H14 M50 16 H64 M14 6 H50 V26 H14 Z", f: "",
                    t: type === "blinkmodul" ? "BL" : type === "tonmodul" ? "TON" : type === "zeitmodul" ? "ZEIT"
                     : type === "mwmodul" ? "MW" : type === "ukwmodul" ? "UKW" : "VST", tx: 32, ty: 16}
        case "feuchtesonde": // zwei Elektroden mit Tropfen dazwischen
            return {s: "M0 16 H24 M40 16 H64 M24 6 V26 M40 6 V26 M32 9 Q36 15 32 19 Q28 15 32 9 Z", f: ""}
        case "kristallhoerer": // Hoerer: Kasten mit gewoelbter Hoermuschel (DIN EN 60617)
            return {s: "M0 16 H22 M42 16 H64 M22 12 H42 V24 H22 Z M22 12 Q32 0 42 12", f: ""}
        case "piezo": // Piezo-Schallgeber: Kristallplatte zwischen zwei Belaegen (DIN EN 60617) mit Schallzeichen
            return {s: "M0 16 H22 M42 16 H64 M22 6 V26 M42 6 V26 M26 8 H38 V24 H26 Z M46 4 Q52 8 46 12", f: "", t: "+", tx: 15, ty: 6}
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
            rotation: -sym.rotation // Schrift bleibt aufrecht, auch wenn das Zeichen senkrecht steht
            color: sym.stroke
            font.pixelSize: sym.shape.t === "+" ? 10 : 13
            font.bold: true
        }
    }
}
