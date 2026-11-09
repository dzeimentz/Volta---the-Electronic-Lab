// Piktogramm eines Moduls (Wiedererkennung neben Name und Plattenfarbe, Farbe nie allein): Blinkmodul Gluehbirne
// mit Strahlen, Tonmodul Note, Verstaerkermodul kleine Welle -> grosse Welle, Zeitmodul Stoppuhr, OP-Modul Dreieck
// mit + und -, Timer 555 Rechteckpuls, Gatter-Modul NAND-Zeichen (Halbkreis mit Kreis am Ausgang), Zaehler-Modul
// Takt mit Treppe, Dekadenzaehler Lauflicht, Anzeige-Modul Siebensegment-Ziffer.
// Gezeichnet in einem Raster der Hoehe 32 (Verstaerker breiter: 48) und auf die Hoehe des Elements skaliert; die
// Breite ergibt sich daraus.
import QtQuick
import QtQuick.Shapes

Item {
    id: icon
    property string type
    property color color: "white"
    readonly property int gridWidth: type === "verstaerkermodul" || type === "timer555" ? 48 : 32
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
        case "timer555":
            return "M 2 24 h 8 v -16 h 10 v 16 h 8 v -16 h 10 v 16 h 8"
        case "gattermodul": // NAND: zwei Eingaenge, Halbkreis, Negationskreis, Ausgang
            return "M 2 10 h 6 M 2 22 h 6 M 8 4 h 8 a 12 12 0 0 1 0 24 h -8 Z"
                 + " M 28 16 a 2.5 2.5 0 1 0 5 0 a 2.5 2.5 0 1 0 -5 0"
        case "zaehlermodul": // Zaehler: Taktimpuls mit Treppe darueber
            return "M 2 29 h 5 v -6 h 5 v 6 h 5 v -6 h 5 v 6 h 5 M 4 17 h 6 v -4 h 6 v -4 h 6 v -4 h 6"
        case "dekadenzaehler": // Lauflicht: zehn Punkte, einer gross
            return "M 2 16 a 1.5 1.5 0 1 0 3 0 a 1.5 1.5 0 1 0 -3 0 M 8 16 a 1.5 1.5 0 1 0 3 0 a 1.5 1.5 0 1 0 -3 0"
                 + " M 13 16 a 3.5 3.5 0 1 0 7 0 a 3.5 3.5 0 1 0 -7 0 M 22 16 a 1.5 1.5 0 1 0 3 0 a 1.5 1.5 0 1 0 -3 0"
                 + " M 27 16 a 1.5 1.5 0 1 0 3 0 a 1.5 1.5 0 1 0 -3 0 M 4 26 h 24"
        case "anzeigemodul": // Siebensegment-Ziffer 8
            return "M 10 3 h 12 M 9 5 v 10 M 23 5 v 10 M 10 16 h 12 M 9 17 v 10 M 23 17 v 10 M 10 29 h 12"
        case "mwmodul": // Antenne mit Wellen (Konzept Volta 3, Kap. 9)
            return "M 16 30 V 12 M 16 12 L 10 3 M 16 12 L 22 3 M 10 3 H 22"
                 + " M 6 10 q -3 4 0 8 M 26 10 q 3 4 0 8 M 2 7 q -5 7 0 14 M 30 7 q 5 7 0 14"
        case "ukwmodul": // Dipolantenne unter einer Welle mit wechselndem Abstand (Frequenzmodulation)
            return "M 16 30 V 17 M 5 17 H 27"
                 + " M 3 8 q 2.5 -6 5 0 t 5 0 q 1.5 -6 3 0 t 3 0 q 1 -6 2 0 t 2 0 q 1.5 -6 3 0 t 3 0"
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
