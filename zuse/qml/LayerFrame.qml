import QtQuick

// Arbeitsflaeche der Zuse-Reihe in drei Schichten (Designsprache Zuse 1 bis 3, docs/Konzept_Zuse.md 6.3):
// links die Eingangsschicht (Bedienfeld: Schalter, Taster), in der Mitte die Logikschicht als Rasterplatine mit
// Steckloechern, in die Bausteine einrasten, rechts die Ausgangsschicht (LEDs). Jede Schicht traegt ihren Namen,
// damit die Gliederung auch ohne Farbe lesbar ist.
Item {
    id: root
    property real grid: 28
    property real logicLeft: 168
    property real logicRight: width - 168
    property color borderColor: ZTheme.accentDim
    property bool showHoles: true

    Rectangle { // Gehaeuse
        anchors.fill: parent
        radius: 14
        color: ZTheme.panelLayer
        border.width: 2
        border.color: root.borderColor
    }
    Rectangle { // Logikschicht: Rasterplatine
        x: root.logicLeft
        y: 2
        width: root.logicRight - root.logicLeft
        height: root.height - 4
        color: ZTheme.board
        Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 2; color: ZTheme.layerEdge }
        Rectangle { anchors { right: parent.right; top: parent.top; bottom: parent.bottom } width: 2; color: ZTheme.layerEdge }
        Canvas { // Steckloecher auf jedem Rasterpunkt; jede vierte Reihe kraeftiger (Orientierung wie auf Lochrasterplatinen)
            anchors.fill: parent
            visible: root.showHoles
            onPaint: {
                const ctx = getContext("2d")
                const g = root.grid
                let x0 = Math.ceil(root.logicLeft / g) * g - root.logicLeft
                if (x0 < 6) x0 += g // nicht auf der Schichtkante
                for (let x = x0; x < width - 6; x += g) {
                    for (let y = g - 2; y < height; y += g) {
                        const strong = Math.round((x + root.logicLeft) / g) % 4 === 0 && Math.round((y + 2) / g) % 4 === 0
                        ctx.beginPath()
                        ctx.arc(x, y, strong ? 3.2 : 2.4, 0, 2 * Math.PI)
                        ctx.fillStyle = strong ? ZTheme.holeStrong : ZTheme.hole
                        ctx.fill()
                    }
                }
            }
        }
    }
    // Schichtnamen
    Repeater {
        model: [
            { text: qsTr("EINGÄNGE"), x: 0, w: root.logicLeft },
            { text: qsTr("LOGIK"), x: root.logicLeft, w: root.logicRight - root.logicLeft },
            { text: qsTr("AUSGÄNGE"), x: root.logicRight, w: root.width - root.logicRight }
        ]
        delegate: Text {
            required property var modelData
            x: modelData.x
            width: modelData.w
            y: 8
            horizontalAlignment: Text.AlignHCenter
            text: modelData.text
            color: ZTheme.muted
            font.pixelSize: 13; font.bold: true
            font.letterSpacing: 2
        }
    }
}
