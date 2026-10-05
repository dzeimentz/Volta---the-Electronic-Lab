// Experimentansicht (docs/ui-spec.md, Lab): Pultleiste oben, Grundplatte, Modulleiste unten.
// Phase-0-Geruest: zeigt die Vorlage eines Experiments mit laufender Simulation und Spannungsanzeigen.
import QtQuick
import QtQuick.Controls
import Volta

Item {
    id: lab
    required property var controller

    // Geometrie der Anschlusspunkte (alle in Koordinaten von 'canvas')
    readonly property var deskNames: ["P:+9V", "P:0V", "P:P1a", "P:P1s", "P:P1b", "P:P2a", "P:P2s", "P:P2b",
                                      "P:S1", "P:S2", "P:LS1", "P:LS2", "MESS:COM", "MESS:VOHM", "MESS:A"]
    readonly property var rows: ["A", "B", "C", "D", "E"]
    readonly property int columns: 8
    readonly property int slots: 16

    readonly property int deskY: 0
    readonly property int boardY: Theme.fieldHeight + 2 * Theme.fieldGap
    readonly property int boardWidth: columns * Theme.fieldWidth + (columns - 1) * Theme.fieldGap
    readonly property int moduleY: boardY + rows.length * (Theme.fieldHeight + Theme.fieldGap) + Theme.fieldGap

    function deskLabel(name) { return name.startsWith("MESS:") ? name.substring(5).replace("VOHM", "V/Ω") : name.substring(2) }

    function anchorPoint(name) {
        let i = deskNames.indexOf(name)
        if (i >= 0)
            return Qt.point(i * (Theme.deskFieldWidth + Theme.deskFieldGap) + Theme.deskFieldWidth / 2,
                            deskY + Theme.fieldHeight / 2)
        if (name.startsWith("M:")) {
            const n = parseInt(name.substring(2)) - 1
            return Qt.point(n * (Theme.slotWidth + Theme.slotGap) + Theme.slotWidth / 2, moduleY + Theme.fieldHeight / 2)
        }
        const r = rows.indexOf(name.charAt(0))
        const c = parseInt(name.substring(1)) - 1
        return Qt.point(c * (Theme.fieldWidth + Theme.fieldGap) + Theme.fieldWidth / 2,
                        boardY + r * (Theme.fieldHeight + Theme.fieldGap) + Theme.fieldHeight / 2)
    }

    // Werkzeugleiste
    Rectangle {
        id: toolbar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: Theme.control + 16
        color: Theme.desk

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: Theme.margin
            spacing: 24

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: lab.controller.title
                color: "white"
                font.pixelSize: 20
                font.bold: true
            }
            Switch {
                id: mainSwitch
                height: Theme.control
                text: qsTr("Hauptschalter")
                checked: lab.controller.mainSwitch
                onToggled: lab.controller.mainSwitch = checked
                palette.windowText: "white"
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Batterie %1 V · Zeit %2 s").arg(lab.controller.batteryVolt.toLocaleString(Qt.locale(), "f", 1))
                                                      .arg(lab.controller.simTime.toLocaleString(Qt.locale(), "f", 1))
                color: Theme.solderAmber
                font.pixelSize: 16
            }
        }
    }

    Rectangle {
        anchors { left: parent.left; right: parent.right; top: toolbar.bottom; bottom: parent.bottom }
        color: Theme.board

        Item {
            id: canvas
            x: Theme.margin * 2
            y: Theme.margin * 2
            width: Math.max(lab.boardWidth, lab.deskNames.length * (Theme.deskFieldWidth + Theme.deskFieldGap))
            height: lab.moduleY + Theme.fieldHeight

            // Pultleiste und Messgeraetebuchsen
            Repeater {
                model: lab.deskNames
                ContactField {
                    required property string modelData
                    anchorName: modelData
                    label: lab.deskLabel(modelData)
                    width: Theme.deskFieldWidth
                    height: Theme.fieldHeight
                    x: lab.anchorPoint(modelData).x - width / 2
                    y: lab.deskY
                    color: modelData.startsWith("MESS:") ? "#D9DEE6" : Theme.field
                }
            }

            // Grundplatte A1-E8
            Repeater {
                model: lab.rows.length * lab.columns
                ContactField {
                    id: boardField
                    required property int index
                    anchorName: lab.rows[Math.floor(index / lab.columns)] + (index % lab.columns + 1)
                    width: Theme.fieldWidth
                    height: Theme.fieldHeight
                    x: lab.anchorPoint(anchorName).x - width / 2
                    y: lab.anchorPoint(anchorName).y - height / 2
                    showVoltage: lab.controller.voltageFields.indexOf(anchorName) >= 0
                    bars: 0
                    Connections {
                        target: lab.controller
                        function onStateChanged() {
                            if (boardField.showVoltage) boardField.bars = lab.controller.voltageBars(boardField.anchorName)
                        }
                    }
                }
            }

            // Modulleiste M1-M16
            Repeater {
                model: lab.slots
                ContactField {
                    required property int index
                    anchorName: "M:" + (index + 1)
                    label: "M" + (index + 1)
                    sockets: 1
                    width: Theme.slotWidth
                    height: Theme.fieldHeight
                    x: lab.anchorPoint(anchorName).x - width / 2
                    y: lab.moduleY
                }
            }

            // Bauteile
            Repeater {
                model: lab.controller.parts
                PartItem {
                    id: partDelegate
                    required property var modelData
                    partId: modelData.id
                    partType: modelData.type
                    value: modelData.value
                    points: modelData.anchors.map(a => lab.anchorPoint(a))
                    Connections {
                        target: lab.controller
                        function onStateChanged() { partDelegate.partState = lab.controller.partState(partDelegate.partId) }
                    }
                }
            }

            // Leitungen
            Repeater {
                model: lab.controller.wires
                WireItem {
                    required property var modelData
                    from: lab.anchorPoint(modelData.from)
                    to: lab.anchorPoint(modelData.to)
                    wireColor: Theme.wireColor(modelData.color)
                }
            }
        }

        // Fehler (Phase 0: zur Entwicklung sichtbar)
        Column {
            anchors { right: parent.right; bottom: parent.bottom; margins: Theme.margin }
            Repeater {
                model: lab.controller.errors
                Label { required property string modelData; text: modelData; color: "#B00020" }
            }
        }
    }
}
