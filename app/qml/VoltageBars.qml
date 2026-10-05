// Spannungsanzeige: vier hellgruene Balken mit Kontur (lesbar auch bei Farbfehlsichtigkeit).
import QtQuick

Row {
    id: root
    property int bars: 0 // 0..4
    spacing: 2

    Repeater {
        model: 4
        Rectangle {
            required property int index
            width: 6
            height: 8 + index * 4
            anchors.bottom: parent.bottom
            radius: 1
            color: index < root.bars ? Theme.voltageBar : "transparent"
            border.color: Theme.voltageBarOutline
            border.width: 1
        }
    }
}
