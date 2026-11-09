// Logikanalysator (ab Volta 3, Stufe 7) unter der Arbeitsflaeche: eine Zeile je gestecktem Logik-Tastkopf LA1-LA8
// mit dem Pegelverlauf im Zeitfenster (neueste Werte rechts). H liegt oben, L unten, der verbotene Bereich ? in der
// Mitte als gestreiftes Band - Lage, Buchstabe und Muster unterscheiden die Pegel, nicht die Farbe allein. Rechts
// steht der aktuelle Pegel als Buchstabe. Bedienung: Zeitfenster fein (20 ms, zeigt das Prellen) oder grob (5 s),
// Standbild. Die Schwellen (70 % und 30 % der Batterie) stehen als Zahl im Kopf. Das Bild rechnet der LabController.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Rectangle {
    id: panel
    required property var controller
    color: "white"
    border.color: Theme.fieldOutline
    property var ls: ({channels: []})
    property bool frozen: false
    readonly property var chans: ls.channels || []
    readonly property real rowH: 26

    function refresh() { panel.ls = panel.controller.logicState() }
    Timer {
        interval: 66
        running: panel.visible && !panel.frozen
        repeat: true
        triggeredOnStart: true
        onTriggered: panel.refresh()
    }

    implicitHeight: head.height + Math.max(1, chans.length) * rowH + 14

    Row {
        id: head
        x: 8; y: 4
        spacing: 8
        height: Theme.control
        Label {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Logikanalysator")
            font.bold: true
            font.pixelSize: Math.round(14 * Theme.fontScale)
            color: Theme.navy
        }
        Button {
            objectName: "logicFine"
            height: Theme.control
            checkable: true
            checked: panel.ls.fine === true
            text: panel.ls.fine === true ? qsTr("fein: %1").arg(panel.ls.windowText || "") : qsTr("grob: %1").arg(panel.ls.windowText || "")
            onClicked: panel.controller.setLogicFine(!(panel.ls.fine === true))
            Accessible.name: qsTr("Zeitfenster umschalten")
        }
        Button {
            objectName: "logicFreeze"
            height: Theme.control
            checkable: true
            checked: panel.frozen
            text: panel.frozen ? qsTr("Weiter") : qsTr("Standbild")
            onClicked: panel.frozen = !panel.frozen
        }
        Label {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("H über %1 · L unter %2 · dazwischen ?").arg(panel.ls.highText || "").arg(panel.ls.lowText || "")
            font.pixelSize: Math.round(12 * Theme.fontScale)
            color: "#4A5866"
        }
    }

    Column {
        id: rows
        x: 8
        anchors.top: head.bottom; anchors.topMargin: 4
        width: panel.width - 16
        Repeater {
            model: panel.chans
            Item {
                id: row
                required property var modelData
                required property int index
                width: rows.width
                height: panel.rowH
                Label {
                    id: name
                    width: 44
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.modelData.label
                    font.bold: true
                    font.pixelSize: Math.round(12 * Theme.fontScale)
                    color: Theme.navy
                }
                Rectangle {
                    id: lane
                    x: name.width
                    width: row.width - name.width - 30
                    height: row.height - 4
                    y: 2
                    color: index % 2 ? "#F4F7FA" : "#EDF2F6"
                    clip: true
                    readonly property var segs: row.modelData.segs || []
                    // verbotener Bereich: gestreiftes Band in der Mitte
                    Repeater {
                        model: Math.floor(lane.segs.length / 3)
                        Rectangle {
                            required property int index
                            readonly property real x0: lane.segs[3 * index] * lane.width
                            readonly property real x1: lane.segs[3 * index + 1] * lane.width
                            visible: lane.segs[3 * index + 2] === 1
                            x: x0; width: Math.max(1, x1 - x0)
                            y: lane.height * 0.3; height: lane.height * 0.4
                            color: "#F2C230"
                            border.color: "#8A6A00"; border.width: 1
                        }
                    }
                    Shape {
                        anchors.fill: parent
                        preferredRendererType: Shape.CurveRenderer
                        ShapePath {
                            strokeColor: "#1F6F3A"
                            strokeWidth: 2
                            fillColor: "transparent"
                            joinStyle: ShapePath.MiterJoin
                            PathPolyline {
                                path: {
                                    const s = lane.segs, w = lane.width, h = lane.height, pts = []
                                    const yOf = lv => lv === 2 ? 2 : lv === 0 ? h - 2 : h / 2
                                    for (let i = 0; i + 2 < s.length; i += 3) {
                                        if (s[i + 2] === 1) continue // verbotener Bereich: nur das Band
                                        pts.push(Qt.point(s[i] * w, yOf(s[i + 2])))
                                        pts.push(Qt.point(s[i + 1] * w, yOf(s[i + 2])))
                                    }
                                    return pts.length > 1 ? pts : [Qt.point(0, h - 2), Qt.point(0, h - 2)]
                                }
                            }
                        }
                    }
                }
                Label {
                    x: lane.x + lane.width + 6
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.modelData.level === 2 ? "H" : row.modelData.level === 0 ? "L" : row.modelData.level === 1 ? "?" : "–"
                    font.bold: true
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    color: "#11161C"
                }
            }
        }
    }
    Label {
        visible: panel.chans.length === 0
        x: 8; anchors.top: head.bottom
        text: qsTr("Stecke einen Logik-Tastkopf auf ein Feld.")
        color: "#4A5866"
    }
}
