import QtQuick
import QtQuick.Shapes

// Schaltzeichen eines Gatters: DIN EN 60617-12 (Rechteck mit &, ≥1, =1, 1) oder IEEE Std 91 (US-Form).
// Die Form folgt aus dem DIN-Symbol; Bausteine ohne Gatterform (z. B. HA) bleiben in beiden Darstellungen Rechtecke.
Item {
    id: root
    property string symbol: "&"
    property bool inverted: false
    property bool us: false
    property color stroke: ZTheme.line
    property real strokeWidth: 2
    property color fill: ZTheme.surface
    property int fontSize: 34

    readonly property string usKind: symbol === "&" ? "and" : symbol === "≥1" ? "or" : symbol === "=1" ? "xor"
                                     : symbol === "1" ? "buf" : ""
    readonly property bool drawUs: us && usKind !== ""
    // Zeichenflaeche (US): Platz fuer die Anschlussleitungen links und den Negationskreis rechts
    readonly property real x0: width * 0.14
    readonly property real x1: width - (inverted ? width * 0.2 : width * 0.1)
    readonly property real y0: height * 0.12
    readonly property real y1: height * 0.88
    readonly property real ym: height / 2
    readonly property real orBack: usKind === "xor" ? x0 + width * 0.07 : x0 // Rueckseite ODER/XODER
    readonly property real orBulge: orBack + (x1 - orBack) * 0.6           // Woelbung der Seiten
    readonly property bool oneInput: usKind === "buf"

    // ---- DIN: Rechteck mit Symbol ----
    Rectangle {
        visible: !root.drawUs
        anchors.fill: parent
        radius: ZTheme.radius
        color: root.fill
        border.width: root.strokeWidth
        border.color: root.stroke
        Text {
            anchors.centerIn: parent
            text: root.symbol
            color: ZTheme.text
            font.family: ZTheme.mono
            font.pixelSize: root.symbol.length > 2 ? root.fontSize * 0.7 : root.fontSize
            font.bold: true
        }
    }
    Rectangle { // Negation am Rechteck (innen vor dem Ausgang)
        visible: !root.drawUs && root.inverted
        width: root.fontSize * 0.47; height: width; radius: width / 2
        x: root.width - width - root.width * 0.1
        y: root.height / 2 - height / 2
        color: root.fill
        border.width: root.strokeWidth
        border.color: root.stroke
    }

    // ---- US: Gatterform nach IEEE Std 91 ----
    Shape {
        visible: root.drawUs
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath { // Anschlussleitungen bis an die Form (liegen unter der Fuellung)
            strokeWidth: root.strokeWidth
            strokeColor: root.stroke
            fillColor: "transparent"
            startX: 0; startY: root.oneInput ? root.ym : root.height * 0.25
            PathLine { x: root.x0 + root.width * 0.12; y: root.oneInput ? root.ym : root.height * 0.25 }
            PathMove { x: 0; y: root.oneInput ? root.ym : root.height * 0.75 }
            PathLine { x: root.x0 + root.width * 0.12; y: root.oneInput ? root.ym : root.height * 0.75 }
            PathMove { x: root.x1; y: root.ym }
            PathLine { x: root.width; y: root.ym }
        }
        ShapePath { // UND: gerade Rueckseite, Halbkreis vorn
            strokeWidth: root.usKind === "and" ? root.strokeWidth : 0
            strokeColor: root.usKind === "and" ? root.stroke : "transparent"
            fillColor: root.usKind === "and" ? root.fill : "transparent"
            startX: root.x0; startY: root.y0
            PathLine { x: root.x0 + (root.x1 - root.x0) * 0.4; y: root.y0 }
            PathArc {
                x: root.x0 + (root.x1 - root.x0) * 0.4; y: root.y1
                radiusX: (root.x1 - root.x0) * 0.6; radiusY: (root.y1 - root.y0) / 2
                useLargeArc: false
            }
            PathLine { x: root.x0; y: root.y1 }
            PathLine { x: root.x0; y: root.y0 }
        }
        ShapePath { // ODER / XODER: gewoelbte Rueckseite, Spitze vorn
            strokeWidth: root.usKind === "or" || root.usKind === "xor" ? root.strokeWidth : 0
            strokeColor: root.usKind === "or" || root.usKind === "xor" ? root.stroke : "transparent"
            fillColor: root.usKind === "or" || root.usKind === "xor" ? root.fill : "transparent"
            startX: root.orBack; startY: root.y0
            PathQuad { x: root.x1; y: root.ym; controlX: root.orBulge; controlY: root.y0 }
            PathQuad { x: root.orBack; y: root.y1; controlX: root.orBulge; controlY: root.y1 }
            PathQuad { x: root.orBack; y: root.y0; controlX: root.orBack + (root.x1 - root.orBack) * 0.25; controlY: root.ym }
        }
        ShapePath { // XODER: zusaetzlicher Bogen hinten
            strokeWidth: root.usKind === "xor" ? root.strokeWidth : 0
            strokeColor: root.usKind === "xor" ? root.stroke : "transparent"
            fillColor: "transparent"
            startX: root.x0; startY: root.y0
            PathQuad { x: root.x0; y: root.y1; controlX: root.x0 + (root.x1 - root.x0) * 0.28; controlY: root.ym }
        }
        ShapePath { // Treiber / NICHT: Dreieck
            strokeWidth: root.usKind === "buf" ? root.strokeWidth : 0
            strokeColor: root.usKind === "buf" ? root.stroke : "transparent"
            fillColor: root.usKind === "buf" ? root.fill : "transparent"
            startX: root.x0 + root.width * 0.1; startY: root.y0 + root.height * 0.06
            PathLine { x: root.x1; y: root.ym }
            PathLine { x: root.x0 + root.width * 0.1; y: root.y1 - root.height * 0.06 }
            PathLine { x: root.x0 + root.width * 0.1; y: root.y0 + root.height * 0.06 }
        }
    }
    Rectangle { // Negationskreis (US) direkt an der Spitze
        visible: root.drawUs && root.inverted
        width: root.width * 0.1; height: width; radius: width / 2
        x: root.x1
        y: root.ym - height / 2
        color: root.fill
        border.width: root.strokeWidth
        border.color: root.stroke
    }
}
