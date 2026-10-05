// Kontaktfeld (Grundplatte, Pultleiste, Messgeraet, Modulleiste): ein Knoten mit vier Steckplaetzen.
// Von einem Feld aus wird eine Leitung gezogen; langer Druck/Rechtsklick oeffnet das Feldmenue.
import QtQuick

Rectangle {
    id: root
    property string anchorName: ""
    property string label: anchorName
    property bool showVoltage: false
    property int bars: 0
    property int sockets: 4 // Steckplaetze (Modulleiste: schmale Felder mit einem Steckplatz)
    property bool highlighted: false

    signal wireDragUpdated(point scenePos)
    signal wireDragFinished(point scenePos)
    signal menuRequested(point scenePos)

    radius: 6
    color: Theme.field
    border.color: highlighted ? Theme.signalOrange : Theme.fieldOutline
    border.width: highlighted ? 4 : 1

    Grid {
        anchors.left: parent.left
        anchors.leftMargin: root.sockets > 1 ? 8 : (parent.width - 6) / 2
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        columns: root.sockets
        spacing: 6
        Repeater {
            model: root.sockets
            Rectangle { width: 6; height: 6; radius: 3; color: Theme.fieldOutline }
        }
    }

    Text {
        anchors.top: parent.top
        anchors.topMargin: 3
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.label
        font.pixelSize: 13
        font.bold: true
        color: Theme.navy
    }

    VoltageBars {
        visible: root.showVoltage
        bars: root.bars
        anchors.right: parent.right
        anchors.rightMargin: 5
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
    }

    property point lastScenePos
    DragHandler {
        id: wireDrag
        target: null
        onCentroidChanged: if (active) { root.lastScenePos = centroid.scenePosition; root.wireDragUpdated(root.lastScenePos) }
        onActiveChanged: if (!active) root.wireDragFinished(root.lastScenePos)
    }
    TapHandler {
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onLongPressed: root.menuRequested(point.scenePosition)
        onTapped: (eventPoint, button) => { if (button === Qt.RightButton) root.menuRequested(eventPoint.scenePosition) }
    }
}
