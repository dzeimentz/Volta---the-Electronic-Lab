// Kontaktfeld (Grundplatte, Pultleiste, Messgeraet, Modulleiste): ein Knoten mit fuenf Steckplaetzen.
// Von einem Feld aus wird eine Leitung gezogen; langer Druck/Rechtsklick oeffnet das Feldmenue.
import QtQuick

Rectangle {
    id: root
    property string anchorName: ""
    property string label: anchorName
    property bool showVoltage: false
    property int bars: 0
    property int sockets: 5 // Steckplaetze (Modulleiste: schmale Felder mit einem Steckplatz)
    property int pitch: Theme.socketPitch // Lochabstand (Lab.socketPoint rechnet mit demselben Wert)
    property bool highlighted: false

    signal wireDragUpdated(point scenePos)
    signal wireDragFinished(point scenePos)
    signal menuRequested(point scenePos)

    radius: 6
    color: Theme.field
    border.color: highlighted ? Theme.signalOrange : Theme.fieldOutline
    border.width: highlighted ? 4 : 1

    // Steckloecher mittig im Feld (waagrecht und senkrecht); Bauteilbeine und Leitungen stecken genau hier
    Repeater {
        model: root.sockets
        Rectangle {
            required property int index
            width: Theme.socketSize
            height: width
            radius: width / 2
            x: root.width / 2 + (index - (root.sockets - 1) / 2) * root.pitch - width / 2
            y: root.height / 2 - height / 2
            color: "#6E6553"
            border.color: "#4F4839"
            border.width: 1
        }
    }

    Text {
        anchors.top: parent.top
        anchors.topMargin: 3
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.label
        font.pixelSize: Math.round((13) * Theme.fontScale)
        font.bold: true
        color: Theme.navy
    }

    VoltageBars {
        visible: root.showVoltage
        bars: root.bars
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2 // unter der Lochreihe
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
