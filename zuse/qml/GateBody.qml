import QtQuick

// Koerper eines Bausteins: Schaltzeichen nach DIN EN 60617 (&, ≥1, =1, 1), Negationskreis am Ausgang,
// eigene Chips mit Stern und Anschlussnamen.
Rectangle {
    id: root
    property var part: ({})
    property bool selected: false
    radius: ZTheme.radius
    color: ZTheme.surface
    border.width: selected ? 3 : 2
    border.color: selected ? ZTheme.accent : part.own ? ZTheme.accentDim : ZTheme.line

    Text {
        anchors.centerIn: parent
        text: root.part.symbol || ""
        color: ZTheme.text
        font.family: ZTheme.mono
        font.pixelSize: (root.part.symbol || "").length > 2 ? 24 : 34
        font.bold: true
    }
    Text { // Stern: selbst gebauter Chip
        visible: !!root.part.own
        anchors { top: parent.top; right: parent.right; margins: 6 }
        text: "★ " + (root.part.type || "")
        color: ZTheme.accent
        font.family: ZTheme.mono
        font.pixelSize: 12
    }
    Rectangle { // Negation (innen vor dem Ausgang, das Loetpad sitzt auf der Kante)
        visible: !!root.part.inverted
        width: 16; height: 16; radius: 8
        x: parent.width - 34; y: parent.height / 2 - 8
        color: ZTheme.surface
        border.width: 2
        border.color: root.border.color
    }
    Repeater { // Anschlussnamen innen
        model: (root.part.inPins || []).concat(root.part.outPins || [])
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
