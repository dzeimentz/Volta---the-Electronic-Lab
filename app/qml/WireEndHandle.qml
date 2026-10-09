// Greifbares Ende einer Leitung: ziehen zum Umsetzen, Antippen waehlt die Leitung (Loeschen-Knopf erscheint),
// langer Druck oder Rechtsklick fuer das Leitungsmenue.
// Fangbereich 48 dp (Bedienelemente >= 48 dp), sichtbar ist nur der Stecker.
import QtQuick

Item {
    id: handle
    property point at
    property color wireColor
    property bool movable: true // Steckmodus: Ende umsetzbar
    signal moved(point scenePos)
    signal released(point scenePos)
    signal menuRequested(point scenePos)
    signal tapped()

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
        enabled: handle.movable
        target: null
        onCentroidChanged: if (active) { handle.lastScenePos = centroid.scenePosition; handle.moved(handle.lastScenePos) }
        onActiveChanged: if (!active) handle.released(handle.lastScenePos)
    }
    TapHandler {
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        gesturePolicy: TapHandler.ReleaseWithinBounds // Antippen gehoert der Leitung, nicht der Flaeche darunter
        onLongPressed: handle.menuRequested(point.scenePosition)
        onTapped: (eventPoint, button) => {
            if (button === Qt.RightButton) handle.menuRequested(eventPoint.scenePosition)
            else handle.tapped()
        }
    }
}
