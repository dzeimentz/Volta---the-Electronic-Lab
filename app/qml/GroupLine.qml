// Gruppenlinie ueber den Pultfeldern eines Bauteils (docs/domain-model.md): duenn, grau, ohne Anschlusspunkte -
// ausdruecklich keine Leitung. In der Mitte das Schaltzeichen; Tippen hebt die Felder der Gruppe hervor.
import QtQuick

Item {
    id: line
    property real fromX
    property real toX
    property string symbol
    signal tapped()

    x: fromX
    width: toX - fromX
    height: 24

    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: 1.5
        color: "#9A9A9A"
    }
    Rectangle {
        anchors.centerIn: parent
        width: symbolText.implicitWidth + 12
        height: 22
        radius: 4
        color: Theme.board
        border.color: "#9A9A9A"
        Text {
            id: symbolText
            anchors.centerIn: parent
            text: line.symbol
            font.pixelSize: 13
            font.bold: true
            color: "#5A5A5A"
        }
    }
    TapHandler { onTapped: line.tapped() }
}
