// Bauteilfach: Bauteile aus dem Fach auf die Grundplatte ziehen (docs/ui-spec.md, Gesten).
// Zwei Spalten, damit alle Bauteile ohne Blaettern sichtbar sind (Ziehen gehoert dem Bauteil, nicht der Liste).
import QtQuick
import QtQuick.Controls

Rectangle {
    id: tray
    required property var parts // catalogParts des LabController
    signal dragUpdated(string type, point scenePos)
    signal dragFinished(string type, point scenePos)

    color: Theme.desk
    width: 2 * 92 + 3 * 8

    Label {
        id: heading
        text: qsTr("Bauteilfach")
        color: "white"
        font.pixelSize: 16
        font.bold: true
        anchors { top: parent.top; left: parent.left; margins: 12 }
    }

    Grid {
        anchors { top: heading.bottom; topMargin: 10; left: parent.left; leftMargin: 8 }
        columns: 2
        spacing: 8

        Repeater {
            model: tray.parts.filter(p => p.model !== "ldr")
            delegate: Rectangle {
                id: entry
                required property var modelData
                objectName: "tray_" + modelData.type
                width: 92
                height: Theme.control + 4
                radius: 8
                color: drag.active ? Theme.solderAmber : "#E9D7B0"
                border.color: Theme.navy

                Label {
                    anchors.fill: parent
                    anchors.margins: 4
                    text: Texts.partName(entry.modelData.type)
                    color: Theme.navy
                    font.pixelSize: 12
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
