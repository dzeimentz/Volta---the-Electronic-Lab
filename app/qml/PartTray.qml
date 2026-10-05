// Bauteilfach: Bauteile aus dem Fach auf die Grundplatte ziehen (docs/ui-spec.md, Gesten).
// Zwei Spalten, damit alle Bauteile ohne Blaettern sichtbar sind (Ziehen gehoert dem Bauteil, nicht der Liste).
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: tray
    required property var parts // catalogParts des LabController
    signal dragUpdated(string type, point scenePos)
    signal dragFinished(string type, point scenePos)
    signal lockedTapped(string type, int stage) // gesperrtes Bauteil angetippt (Hinweis)

    color: Theme.desk
    width: 2 * 92 + 3 * 8

    Label {
        id: heading
        text: qsTr("Bauteilfach")
        color: "white"
        font.pixelSize: Math.round((16) * Theme.fontScale)
        font.bold: true
        anchors { top: parent.top; left: parent.left; margins: 12 }
    }

    Grid {
        anchors { top: heading.bottom; topMargin: 10; left: parent.left; leftMargin: 8 }
        columns: 2
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
                readonly property bool locked: modelData.locked === true
                color: locked ? "#8E9AA8" : (drag.active ? Theme.solderAmber : "#E9D7B0")
                border.color: locked ? "#5D6B7A" : Theme.navy
                opacity: locked ? 0.75 : 1

                // Schaltzeichen nach Norm (DIN EN 60617) ueber dem Namen
                SchematicSymbol {
                    objectName: "symbol_" + entry.modelData.type
                    type: entry.modelData.type
                    stroke: entry.locked ? "#3E4955" : Theme.navy
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
                    color: entry.locked ? "#26313C" : Theme.navy
                    font.pixelSize: Math.round((12) * Theme.fontScale)
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                // Schloss zusaetzlich zur grauen Farbe (Farbe nie allein)
                Label {
                    visible: entry.locked
                    x: parent.width - width - 4; y: 2
                    text: "🔒" + entry.modelData.stage // Stufe, ab der das Bauteil frei wird
                    font.pixelSize: 13
                    font.bold: true
                    color: "#1E2833"
                }
                TapHandler {
                    enabled: entry.locked
                    onTapped: tray.lockedTapped(entry.modelData.type, entry.modelData.stage)
                }
                Accessible.name: Texts.partName(entry.modelData.type) + (entry.locked ? ", " + qsTr("gesperrt bis Stufe %1").arg(entry.modelData.stage) : "")

                property point lastScenePos
                DragHandler {
                    id: drag
                    enabled: !entry.locked
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
