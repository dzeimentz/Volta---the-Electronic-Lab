// Bauteil zwischen seinen Feldern (Phase-0-Geruest: schematische Darstellung; Gestaltung folgt in Phase 1).
// Zustand: ueberlastet = orange Glut, zerstoert = grau mit Kennzeichnung (Farbe nie allein: Text + Kontur).
import QtQuick

Item {
    id: root
    property string partId
    property string partType
    property real value: 0
    property var points: []      // Mittelpunkte der Felder je Anschluss
    property var partState: ({}) // partState() des LabController

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

    function valueText() {
        if (partType === "widerstand") return value >= 1000 ? (value / 1000) + " kΩ" : value + " Ω"
        return ""
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
        height: 28
        radius: 8
        x: root.cx - width / 2
        y: root.cy - height / 2
        rotation: Math.atan2(root.b.y - root.a.y, root.b.x - root.a.x) * 180 / Math.PI
        color: root.destroyed ? Theme.destroyed
             : root.overloaded ? Theme.signalOrange
             : root.partType.startsWith("led") ? "#E8221B"
             : "#E9D7B0"
        border.color: root.overloaded ? "#B23A00" : Theme.navy
        border.width: root.overloaded ? 3 : 1

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
                             : root.partId + (root.valueText() ? " " + root.valueText() : "")
        font.pixelSize: 12
        color: Theme.navy
    }
}
