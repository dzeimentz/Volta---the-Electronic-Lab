// Bauteilfach: Bauteile aus dem Fach auf die Grundplatte ziehen (docs/ui-spec.md, Gesten).
// Zwei Spalten, damit alle Bauteile ohne Blaettern sichtbar sind (Ziehen gehoert dem Bauteil, nicht der Liste).
// Hochformat (horizontal): Leiste unter der Grundplatte, so viele Spalten wie in die Breite passen.
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: tray
    required property var parts // catalogParts des LabController
    property bool horizontal: false
    signal dragUpdated(string type, point scenePos)
    signal dragFinished(string type, point scenePos)

    color: Theme.desk
    implicitWidth: 2 * 92 + 3 * 8 // senkrecht: zwei Spalten
    implicitHeight: grid.y + grid.height + 10 // waagrecht: so hoch wie die Reihen

    Label {
        id: heading
        text: qsTr("Bauteilfach")
        color: "white"
        font.pixelSize: Math.round((16) * Theme.fontScale)
        font.bold: true
        anchors { top: parent.top; left: parent.left; margins: 12 }
    }

    Grid {
        id: grid
        anchors { top: heading.bottom; topMargin: 10; left: parent.left; leftMargin: 8 }
        columns: tray.horizontal ? Math.max(2, Math.floor((tray.width - 16 + 6) / (92 + 6))) : 2
        spacing: 6

        Repeater {
            model: tray.parts.filter(p => p.model !== "ldr")
            delegate: Rectangle {
                id: entry
                required property var modelData
                objectName: "tray_" + modelData.type
                width: 92
                height: 60
                radius: 8
                color: drag.active ? Theme.solderAmber : "#E9D7B0"
                border.color: Theme.navy

                // Schaltzeichen nach Norm (DIN EN 60617) ueber dem Namen
                SchematicSymbol {
                    objectName: "symbol_" + entry.modelData.type
                    type: entry.modelData.type
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 5
                    width: 52
                    height: 22
                }
                Label {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 3
                    height: 26
                    text: Texts.partName(entry.modelData.type)
                    color: Theme.navy
                    font.pixelSize: Math.round((12) * Theme.fontScale)
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                property point lastScenePos
                DragHandler {
                    id: drag
                    target: null
                    onCentroidChanged: {
                        if (active) {
                            entry.lastScenePos = centroid.scenePosition
                            tray.dragUpdated(entry.modelData.type, entry.lastScenePos)
                        }
                    }
                    onActiveChanged: if (!active) tray.dragFinished(entry.modelData.type, entry.lastScenePos)
                }
            }
        }
    }
}
