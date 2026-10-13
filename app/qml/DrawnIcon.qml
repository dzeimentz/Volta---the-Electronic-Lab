// Gezeichnete Symbole statt Schriftzeichen: Menue, Pfeil auf/ab, Schliessen, Rueckgaengig, Wiederholen, Loeschen,
// Stift. Diese Zeichen (U+2630, U+25BE, U+25B4, U+2715, U+21B6/7, U+232B, U+270E) fehlen in den Schriften, die Qt auf
// Android nutzt (leere Kaestchen); gezeichnet sehen sie auf allen Geraeten gleich aus. Zeichenflaeche 24 x 24.
import QtQuick
import QtQuick.Shapes

Item {
    id: icon
    property string kind: "menu" // menu, down, up, left, right, close, undo, redo, backspace, pencil, trash, fit
    property color color: "white"
    property real size: 20

    implicitWidth: size
    implicitHeight: size

    readonly property bool filled: kind === "down" || kind === "up" || kind === "left" || kind === "right"
    readonly property string path: ({
        menu: "M4 6 H20 M4 12 H20 M4 18 H20",
        down: "M6 9 L18 9 L12 16 Z",
        up: "M6 15 L18 15 L12 8 Z",
        left: "M15 6 L15 18 L8 12 Z",
        right: "M9 6 L9 18 L16 12 Z",
        close: "M6 6 L18 18 M18 6 L6 18",
        undo: "M8 5 L4 9 L8 13 M4 9 H14 A5 5 0 0 1 14 19 H9",
        redo: "M16 5 L20 9 L16 13 M20 9 H10 A5 5 0 0 0 10 19 H15",
        backspace: "M8 6 H20 V18 H8 L3 12 Z M11 9 L17 15 M17 9 L11 15",
        pencil: "M4 20 L5 15 L16 4 L20 8 L9 19 Z M14 6 L18 10",
        trash: "M4 7 H20 M9 7 V4 H15 V7 M6 7 L7 20 H17 L18 7 M10 11 V17 M14 11 V17",
        fit: "M4 9 V4 H9 M15 4 H20 V9 M20 15 V20 H15 M9 20 H4 V15 M8 8 H16 V16 H8 Z"
    })[kind] || ""

    Shape {
        width: 24
        height: 24
        scale: icon.size / 24
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: icon.color
            strokeWidth: icon.filled ? 1 : 2.2
            fillColor: icon.filled ? icon.color : "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: icon.path }
        }
    }
}
