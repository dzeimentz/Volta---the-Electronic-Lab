// Bauteil zwischen seinen Feldern (Gestaltung folgt in Schritt 1.4).
// Zustand: ueberlastet = orange Glut, zerstoert = grau mit Kennzeichnung (Farbe nie allein: Text + Kontur).
// Bedienung: ziehen zum Verschieben, Tippen waehlt aus (Taster: druecken schliesst), langer Druck/Rechtsklick Menue.
import QtQuick

Item {
    id: root
    property string partId
    property string partType
    property real value: 0
    property bool closed: false
    property bool selected: false
    property var points: []      // Mittelpunkte der Felder je Anschluss
    property var partState: ({}) // partState() des LabController

    signal dragFinished(point delta)
    signal tapped()
    signal pushed(bool down) // Taster gedrueckt/losgelassen
    signal menuRequested(point scenePos)

    anchors.fill: parent

    readonly property point a: points.length > 0 ? points[0] : Qt.point(0, 0)
    readonly property point b: points.length > 1 ? points[points.length - 1] : a
    readonly property real cx: {
        let s = 0
        for (let i = 0; i < points.length; ++i) s += points[i].x
        return points.length > 0 ? s / points.length : 0
    }
    readonly property real cy: {
        let s = 0
        for (let i = 0; i < points.length; ++i) s += points[i].y
        return points.length > 0 ? s / points.length : 0
    }
    readonly property bool destroyed: partState.destroyed === true
    readonly property bool overloaded: partState.overloaded === true

    // Darstellung (wandert beim Ziehen mit; der Griffbereich darunter bleibt stehen)
    Item {
    id: visual
    anchors.fill: parent
    transform: Translate {
        x: bodyDrag.active ? bodyDrag.activeTranslation.x : 0
        y: bodyDrag.active ? bodyDrag.activeTranslation.y : 0
    }

    // Beine
    Repeater {
        model: root.points
        Rectangle {
            required property var modelData
            readonly property real dx: modelData.x - root.cx
            readonly property real dy: modelData.y - root.cy
            x: root.cx
            y: root.cy - 1.5
            width: Math.sqrt(dx * dx + dy * dy)
            height: 3
            color: "#8A8A8A"
            transformOrigin: Item.Left
            rotation: Math.atan2(dy, dx) * 180 / Math.PI
        }
    }

    // Koerper
    Rectangle {
        id: body
        width: 64
        height: 32
        radius: 8
        x: root.cx - width / 2
        y: root.cy - height / 2
        rotation: Math.atan2(root.b.y - root.a.y, root.b.x - root.a.x) * 180 / Math.PI
        color: root.destroyed ? Theme.destroyed
             : root.overloaded ? Theme.signalOrange
             : root.partType.startsWith("led") ? "#E8221B"
             : "#E9D7B0"
        border.color: root.selected ? Theme.accent : (root.overloaded ? "#B23A00" : Theme.navy)
        border.width: root.selected || root.overloaded ? 3 : 1

        // leuchtende LED: Helligkeit aus dem Strom (level = I / 20 mA)
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            visible: root.partType.startsWith("led") && !root.destroyed
            color: "#FFF4B0"
            opacity: Math.min(1, Math.max(0, (root.partState.level || 0) * 0.8))
        }
    }

    Text {
        anchors.horizontalCenter: body.horizontalCenter
        y: body.y + body.height + 4
        text: root.destroyed ? qsTr("%1 kaputt").arg(root.partId)
                             : root.partId + (root.partType === "widerstand" ? " " + Texts.ohm(root.value) : "")
        font.pixelSize: 12
        color: Theme.navy
    }
    }

    // Bedienung auf dem Koerper (Fangbereich mindestens 48 dp), ausserhalb der mitwandernden Darstellung
    Item {
        x: root.cx - width / 2
        y: root.cy - height / 2
        // nicht drehen (Verschiebung in Szenen-Richtung messen), sondern bei senkrechter Lage Masse tauschen
        readonly property bool upright: Math.abs(Math.abs(body.rotation) - 90) < 45
        width: Math.max(upright ? body.height : body.width, Theme.control)
        height: Math.max(upright ? body.width : body.height, Theme.control)

        DragHandler {
            id: bodyDrag
            target: null
            // activeTranslation ist beim Deaktivieren schon zurueckgesetzt: letzte Verschiebung selbst merken
            property point lastTranslation
            onActiveTranslationChanged: if (active) lastTranslation = Qt.point(activeTranslation.x, activeTranslation.y)
            onActiveChanged: if (!active) root.dragFinished(lastTranslation)
        }
        TapHandler {
            id: tap
            // exklusiver Griff: der Tipp gehoert dem Bauteil, nicht dem Hintergrund (der die Auswahl aufhebt);
            // das Ziehen darf ihn dennoch uebernehmen
            gesturePolicy: TapHandler.ReleaseWithinBounds
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onTapped: (eventPoint, button) => {
                if (button === Qt.RightButton) root.menuRequested(eventPoint.scenePosition)
                else root.tapped()
            }
            onLongPressed: root.menuRequested(point.scenePosition)
            onPressedChanged: root.pushed(pressed)
        }
    }
}
