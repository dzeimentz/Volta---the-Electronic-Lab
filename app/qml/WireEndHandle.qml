// Greifbares Ende einer Leitung: ziehen zum Umsetzen, langer Druck oder Rechtsklick fuer das Leitungsmenue.
// Fangbereich 48 dp (Bedienelemente >= 48 dp), sichtbar ist nur der Stecker.
import QtQuick

Item {
    id: handle
    property point at
    property color wireColor
    signal moved(point scenePos)
    signal released(point scenePos)
    signal menuRequested(point scenePos)

    width: Theme.control
    height: Theme.control
    x: at.x - width / 2
    y: at.y - height / 2

    Rectangle {
        anchors.centerIn: parent
        width: drag.active ? 16 : 11 // passt in den Lochabstand (Theme.socketPitch)
        height: width
        radius: width / 2
        color: handle.wireColor
        border.color: Theme.navy
        border.width: 2
    }

    property point lastScenePos
    DragHandler {
        id: drag
        target: null
        onCentroidChanged: if (active) { handle.lastScenePos = centroid.scenePosition; handle.moved(handle.lastScenePos) }
        onActiveChanged: if (!active) handle.released(handle.lastScenePos)
    }
    TapHandler {
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onLongPressed: handle.menuRequested(point.scenePosition)
        onTapped: (eventPoint, button) => { if (button === Qt.RightButton) handle.menuRequested(eventPoint.scenePosition) }
    }
}
