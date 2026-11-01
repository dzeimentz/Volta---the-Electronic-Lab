// Spannungsanzeige: vier hellgruene Balken mit Kontur (lesbar auch bei Farbfehlsichtigkeit). Ab Volta 2 (Konzept 5.4)
// negativ als umgedrehtes blaues Balkendiagramm mit Minuszeichen, Wechselspannung mit »~« (Balken nach dem
// Effektivwert). Farbe nie allein: Richtung und Zeichen tragen dieselbe Information.
import QtQuick

Row {
    id: root
    property int bars: 0 // 0..4
    property bool negative: false
    property bool ac: false
    spacing: 2

    Text {
        visible: root.negative || root.ac
        anchors.verticalCenter: parent.verticalCenter
        text: root.ac ? "~" : "−"
        font.pixelSize: root.ac ? 16 : 14
        font.bold: true
        color: root.negative ? Theme.voltageBarNegativeOutline : "#1E2B38"
    }
    Repeater {
        model: 4
        Rectangle {
            required property int index
            width: 6
            // negativ: der erste Balken ist der hoechste und alle haengen oben (wachsen nach unten)
            height: 8 + (root.negative ? 3 - index : index) * 4
            y: root.negative ? 0 : 20 - height
            radius: 1
            color: index < root.bars ? (root.negative ? Theme.voltageBarNegative : Theme.voltageBar) : "transparent"
            border.color: root.negative ? Theme.voltageBarNegativeOutline : Theme.voltageBarOutline
            border.width: 1
        }
    }
}
