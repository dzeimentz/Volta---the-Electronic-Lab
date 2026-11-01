// Piktogramm eines Moduls (Wiedererkennung neben Name und Plattenfarbe, Farbe nie allein): Blinkmodul Gluehbirne
// mit Strahlen, Tonmodul Note, Verstaerkermodul kleine Welle -> grosse Welle, Zeitmodul Stoppuhr, OP-Modul Dreieck
// mit + und -.
// Gezeichnet in einem Raster der Hoehe 32 (Verstaerker breiter: 48) und auf die Hoehe des Elements skaliert; die
// Breite ergibt sich daraus.
import QtQuick
import QtQuick.Shapes

Item {
    id: icon
    property string type
    property color color: "white"
    readonly property int gridWidth: type === "verstaerkermodul" ? 48 : 32
    implicitHeight: 24
    implicitWidth: height * gridWidth / 32

    readonly property string strokes: {
        switch (type) {
        case "blinkmodul":
            return "M 9 13 a 7 7 0 1 0 14 0 a 7 7 0 1 0 -14 0 M 13 22.5 h6 M 14 26 h4"
                 + " M 2 13 h3 M 27 13 h3 M 6 3.5 l2.2 2.2 M 26 3.5 l-2.2 2.2 M 16 0.5 v2.5"
        case "tonmodul":
            return "M 18.3 22 V 3.5 q 8.5 3 8.5 12.5"
        case "verstaerkermodul":
            return "M 2 16 q 2.5 -7 5 0 t 5 0 M 15 16 h7 M 19 12.5 l3.5 3.5 l-3.5 3.5 M 26 16 q 5 -27 10 0 t 10 0"
        case "opmodul":
            return "M 7 3 L 29 16 L 7 29 Z M 10 10 h5 M 12.5 7.5 v5 M 10 22 h5"
        case "zeitmodul":
            return "M 6 18.5 a 10 10 0 1 0 20 0 a 10 10 0 1 0 -20 0 M 13 3 h6 M 16 3 v5.5 M 16 18.5 l 4.5 -5.5 M 24 9.5 l 2 -2"
        }
        return ""
    }
    readonly property string fills: type === "tonmodul" ? "M 8.3 25 a 5.2 3.9 -20 1 0 10.4 -3.6 a 5.2 3.9 -20 1 0 -10.4 3.6 Z" : ""

    Shape {
        width: icon.gridWidth
        height: 32
        scale: icon.height / 32
        transformOrigin: Item.TopLeft
        x: (icon.width - icon.gridWidth * scale) / 2
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: icon.color
            strokeWidth: 2.6
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: icon.strokes !== "" ? icon.strokes : "M 0 0" }
        }
        ShapePath {
            strokeColor: "transparent"
            fillColor: icon.fills !== "" ? icon.color : "transparent"
            PathSvg { path: icon.fills !== "" ? icon.fills : "M 0 0" }
        }
    }
    Accessible.ignored: true
}
