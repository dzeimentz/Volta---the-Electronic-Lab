// Bauteil wie auf dem Steckbrett, mit Anschlussdraehten, verkleinert auf die verfuegbare Flaeche (Bauteilfach).
// Lage wie auf der Grundplatte: Beine vom Koerper zu den Steckpunkten, Beinenden als Punkte (vgl. PartItem.qml).
import QtQuick

Item {
    id: root
    property string type
    property int pins: 2 // 2 zweipolig, 3 Traegerplatine (Wechselschalter), 4 quadratische Platine

    readonly property bool square: pins === 4
    readonly property bool carrier: pins === 3
    // gedachte Flaeche in Steckbrett-Massen, Steckpunkte und Koerper darin
    readonly property size area: square ? Qt.size(130, 110) : carrier ? Qt.size(200, 40) : Qt.size(124, 40)
    readonly property var points: square ? [Qt.point(17, 15), Qt.point(113, 15), Qt.point(17, 95), Qt.point(113, 95)]
                                 : carrier ? [Qt.point(6, 20), Qt.point(100, 20), Qt.point(194, 20)]
                                 : [Qt.point(6, 20), Qt.point(118, 20)]
    readonly property point center: Qt.point(area.width / 2, area.height / 2)

    Item {
        width: root.area.width
        height: root.area.height
        anchors.centerIn: parent
        scale: Math.min(1, root.width / width, root.height / height)

        // Beine
        Repeater {
            model: root.points
            Rectangle {
                required property var modelData
                readonly property real dx: modelData.x - root.center.x
                readonly property real dy: modelData.y - root.center.y
                x: root.center.x
                y: root.center.y - 1.5
                width: Math.sqrt(dx * dx + dy * dy)
                height: 3
                color: "#8A8A8A"
                transformOrigin: Item.Left
                rotation: Math.atan2(dy, dx) * 180 / Math.PI
            }
        }
        PartBody {
            id: body
            type: root.type
            width: root.square ? 122 : root.carrier ? 160 : 72
            height: root.square ? 102 : 34
            x: root.center.x - width / 2
            y: root.center.y - height / 2
            feet: root.points.map(pt => Qt.point(pt.x - x, pt.y - y))
            value: root.type === "widerstand" ? 1000 : 0 // Farbringe wie ein neuer Widerstand (1 kOhm)
        }
        // Beinenden
        Repeater {
            model: root.points
            Rectangle {
                required property var modelData
                width: 6; height: 6; radius: 3
                x: modelData.x - 3
                y: modelData.y - 3
                color: "#5E5E5E"
                border.color: "#2E2E2E"
                border.width: 1
            }
        }
    }
}
