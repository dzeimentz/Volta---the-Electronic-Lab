// Schaltzeichen nach DIN EN 60617 (IEC 60617) fuer Bauteilfach und spaeter die Schaltplanansicht.
// Gezeichnet in einem Raster von 64 x 32 Einheiten (Anschluesse links und rechts auf halber Hoehe), skaliert auf die
// Groesse des Elements. Einfarbig (Strichfarbe), wie in der Norm; Farbe traegt keine Information.
import QtQuick

Canvas {
    id: sym
    property string type
    property color stroke: Theme.navy
    property real lineWidth: 2

    implicitWidth: 64
    implicitHeight: 32
    onTypeChanged: requestPaint()
    onStrokeChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const c = getContext("2d")
        c.reset()
        const s = Math.min(width / 64, height / 32)
        c.translate((width - 64 * s) / 2, (height - 32 * s) / 2)
        c.scale(s, s)
        c.strokeStyle = sym.stroke
        c.fillStyle = sym.stroke
        c.lineWidth = sym.lineWidth / s * 0.9
        c.lineCap = "round"
        c.lineJoin = "round"

        function line(x1, y1, x2, y2) { c.beginPath(); c.moveTo(x1, y1); c.lineTo(x2, y2); c.stroke() }
        function leads(l, r) { line(0, 16, l, 16); line(r, 16, 64, 16) }
        function arrow(x1, y1, x2, y2) { // Pfeil von (x1,y1) nach (x2,y2)
            line(x1, y1, x2, y2)
            const a = Math.atan2(y2 - y1, x2 - x1)
            c.beginPath()
            c.moveTo(x2, y2)
            c.lineTo(x2 - 4 * Math.cos(a - 0.5), y2 - 4 * Math.sin(a - 0.5))
            c.lineTo(x2 - 4 * Math.cos(a + 0.5), y2 - 4 * Math.sin(a + 0.5))
            c.closePath()
            c.fill()
        }
        function diode() { // Dreieck nach rechts mit Strich (Durchlassrichtung links -> rechts)
            leads(24, 38)
            c.beginPath(); c.moveTo(24, 8); c.lineTo(38, 16); c.lineTo(24, 24); c.closePath(); c.stroke()
            line(38, 8, 38, 24)
        }
        function contactNO(x1, x2) { // Schliesser: Anschlusspunkt links, Kontaktmesser schraeg
            leads(x1, x2)
            line(x1, 16, x2 + 2, 8)
        }

        switch (sym.type) {
        case "widerstand":
            leads(18, 46)
            c.strokeRect(18, 10, 28, 12)
            break
        case "fotowiderstand":
            leads(18, 46)
            c.strokeRect(18, 12, 28, 10)
            arrow(10, 0, 20, 9); arrow(18, 0, 28, 9)
            break
        case "gluehlampe": // Kreis mit Kreuz
            leads(20, 44)
            c.beginPath(); c.arc(32, 16, 12, 0, 2 * Math.PI); c.stroke()
            line(23.5, 7.5, 40.5, 24.5); line(40.5, 7.5, 23.5, 24.5)
            break
        case "motor": // Kreis mit M
            leads(20, 44)
            c.beginPath(); c.arc(32, 16, 12, 0, 2 * Math.PI); c.stroke()
            c.font = "bold 13px sans-serif"
            c.textAlign = "center"; c.textBaseline = "middle"
            c.fillText("M", 32, 16.5)
            break
        case "summer": // Summer: Halbkreis auf einer Grundlinie
            line(0, 24, 20, 24); line(44, 24, 64, 24)
            line(20, 24, 44, 24)
            c.beginPath(); c.arc(32, 24, 12, Math.PI, 2 * Math.PI); c.stroke()
            break
        case "taster": // Schliesser mit Druckbetaetigung
            contactNO(22, 42)
            line(32, 12, 32, 3); line(27, 3, 37, 3)
            c.setLineDash([2, 2]); line(32, 12, 32, 4); c.setLineDash([])
            break
        case "reedkontakt": // Schliesser im Schutzrohr
            contactNO(24, 40)
            c.beginPath(); c.ellipse(14, 4, 36, 24); c.stroke()
            break
        case "schalter":
            contactNO(22, 42)
            break
        case "wechselschalter": // Wechsler: Mitte links, Stellungen rechts oben/unten
            line(0, 16, 22, 16)
            line(42, 8, 64, 8); line(42, 24, 64, 24)
            line(22, 16, 42, 8)
            break
        case "diode_si":
            diode()
            break
        case "led_rot": case "led_gruen": case "led_gelb": case "led_blau": // LED: Diode mit zwei Pfeilen nach aussen
            diode()
            arrow(30, 7, 37, 0); arrow(36, 9, 43, 2)
            break
        case "kondensator":
            leads(29, 35)
            line(29, 6, 29, 26); line(35, 6, 35, 26)
            break
        case "elko": // gepolt: ein Belag hohl (Plus), einer gefuellt
            leads(27, 37)
            c.strokeRect(25, 6, 4, 20)
            c.fillRect(35, 6, 4, 20)
            c.font = "bold 10px sans-serif"; c.textAlign = "center"; c.textBaseline = "middle"
            c.fillText("+", 20, 7)
            break
        case "transistor_npn": // NPN: Basis links, Kollektor oben, Emitter unten mit Pfeil nach aussen
            c.beginPath(); c.arc(34, 16, 14, 0, 2 * Math.PI); c.stroke()
            line(8, 16, 28, 16)        // Basis
            line(28, 8, 28, 24)        // Basisstrich
            line(28, 12, 40, 4); line(40, 4, 40, 0)   // Kollektor
            arrow(28, 20, 39, 27); line(39, 27, 40, 32) // Emitter
            break
        default:
            leads(20, 44)
            c.strokeRect(20, 8, 24, 16)
        }
    }
}
