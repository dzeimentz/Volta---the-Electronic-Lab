import QtQuick

// Koerper eines Bausteins: Schaltzeichen nach DIN EN 60617-12 oder IEEE Std 91 (Umschalter workbench.symbolStyle),
// eigene Chips mit Stern und (in Rechteckform) Anschlussnamen.
Item {
    id: root
    property var part: ({})
    property bool selected: false

    GateShape {
        id: shape
        anchors.fill: parent
        symbol: root.part.symbol || ""
        inverted: !!root.part.inverted
        us: workbench.symbolStyle === "us"
        strokeWidth: root.selected ? 3 : 2
        stroke: root.selected ? ZTheme.accent : root.part.own ? ZTheme.accentDim : ZTheme.muted
    }
    Text { // Stern: selbst gebauter Chip
        visible: !!root.part.own
        anchors { top: parent.top; right: parent.right; margins: shape.drawUs ? -4 : 6 }
        text: "★ " + (root.part.type || "")
        color: ZTheme.accent
        font.family: ZTheme.mono
        font.pixelSize: 12
    }
    Repeater { // Anschlussnamen innen (nur Rechteckform)
        model: shape.drawUs ? [] : (root.part.inPins || []).concat(root.part.outPins || [])
        Text {
            required property var modelData
            visible: modelData.name !== ""
            text: modelData.name
            color: ZTheme.muted
            font.family: ZTheme.mono
            font.pixelSize: 13
            x: modelData.x <= root.part.x + 1 ? 8 : root.width - width - 10
            y: modelData.y - root.part.y - height / 2
        }
    }
}
