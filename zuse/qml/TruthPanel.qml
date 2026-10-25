import QtQuick
import QtQuick.Layouts

// Tabelle der Stufe. Entdecken: zuerst die eigene Vermutung je Schalterstellung (antippen: 1/0), dann was die
// eingesetzte Kiste bei jeder ausprobierten Stellung gemacht hat (aktuelle Stellung umrandet) und ob das zur
// Vermutung passt (= / ≠). Bauen: Soll und nach dem Pruefen Ist; Abweichungen mit ✗ und Rahmen, nicht nur Farbe.
// Schaltungen mit Speicher: Ablauf mit nummerierten Schritten (Text und Schalterstellung darunter).
ColumnLayout {
    id: root
    spacing: 4
    readonly property bool discover: workbench.stageKind === "entdecken"
    readonly property var inputs: workbench.panelInputs
    readonly property var outputs: workbench.panelOutputs
    // Proben grosser Tabellen: statt der Schalterbits steht die Rechnung in der ersten Spalte
    readonly property bool withNotes: workbench.checkRows.length > 0 && (workbench.checkRows[0].note ?? "") !== ""
    readonly property bool sequence: workbench.stageSequence
    readonly property int colWidth: sequence ? 58 : 76
    readonly property int inWidth: sequence ? Math.max(120, width - 2 * colWidth - 60)
                                            : withNotes ? 112 : Math.max(60, inputs.length * 24 + 12)

    // Legende: Buchstabe = Bedeutung, platzsparend nebeneinander; bei vielen Anschluessen weggelassen (die
    // Bezeichnungen stehen dann an den Schaltern und LEDs selbst)
    Flow {
        visible: root.inputs.length + root.outputs.length <= 6
        Layout.fillWidth: true
        spacing: 12
        Repeater {
            model: root.inputs.concat(root.outputs)
            delegate: Text {
                required property var modelData
                visible: modelData.label !== ""
                text: modelData.name + " = " + modelData.label
                color: ZTheme.muted
                font.pixelSize: 14
            }
        }
    }
    Item { Layout.preferredHeight: 2 }
    RowLayout { // Spaltenkoepfe
        Layout.fillWidth: true
        Layout.leftMargin: 10
        spacing: 6
        Text {
            Layout.preferredWidth: root.inWidth
            text: root.sequence ? qsTr("Schritt") + "  (" + root.inputs.map(p => p.name).join(" ") + ")"
                                : root.withNotes ? qsTr("Rechnung") : root.inputs.map(p => p.name).join(" ")
            color: ZTheme.muted; font.family: ZTheme.mono; font.pixelSize: 15; font.bold: true
        }
        Text {
            Layout.preferredWidth: root.colWidth
            text: root.discover ? qsTr("Vermutung") : qsTr("soll")
            color: ZTheme.muted; font.pixelSize: 14; font.bold: true
        }
        Text {
            Layout.preferredWidth: root.colWidth
            text: root.discover ? qsTr("Kiste") : qsTr("ist")
            color: ZTheme.muted; font.pixelSize: 14; font.bold: true
        }
    }
    Repeater {
        model: workbench.checkRows
        delegate: Rectangle {
            id: row
            required property var modelData
            required property int index
            readonly property bool filled: modelData.actual !== ""
            readonly property bool wrong: !root.discover && filled && !modelData.ok
            Layout.fillWidth: true
            implicitHeight: root.sequence ? Math.max(34, stepCol.implicitHeight + 10)
                                          : root.discover && !workbench.stageDone ? 48 : 34 // antippbar: 48 dp
            radius: 8
            color: root.discover ? (filled ? "#ECFAFD" : ZTheme.surfaceHigh)
                                 : (filled ? (modelData.ok ? ZTheme.okBg : ZTheme.badBg) : ZTheme.surfaceHigh)
            border.width: wrong || (root.discover && !!modelData.current) ? 2 : 0
            border.color: wrong ? ZTheme.bad : ZTheme.accent
            TapHandler { // ganze Zeile (48 dp) als Ziel fuer die Vermutung
                enabled: root.discover && !workbench.stageDone
                onTapped: workbench.toggleGuess(row.index)
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 8
                spacing: 6
                Text {
                    visible: !root.sequence
                    Layout.preferredWidth: root.inWidth
                    text: root.withNotes ? row.modelData.note : row.modelData.inputs.split("").join(" ")
                    color: ZTheme.text; font.family: ZTheme.mono; font.pixelSize: 17; font.bold: true
                }
                ColumnLayout { // Ablauf: Schritt in Worten, darunter die Stellung der Schalter und Taster
                    id: stepCol
                    visible: root.sequence
                    Layout.preferredWidth: root.inWidth
                    spacing: 0
                    Text {
                        Layout.fillWidth: true
                        text: row.modelData.note ?? ""
                        color: ZTheme.text; font.pixelSize: 14; font.bold: true
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        text: row.modelData.inputs.split("").join(" ")
                        color: ZTheme.muted; font.family: ZTheme.mono; font.pixelSize: 13
                    }
                }
                Item { // Soll (bauen) bzw. Vermutung (entdecken, antippbar solange die Stufe offen ist)
                    Layout.preferredWidth: root.colWidth
                    Layout.fillHeight: true
                    Rectangle {
                        id: guessCell
                        readonly property bool editable: root.discover && !workbench.stageDone
                        visible: editable
                        anchors.verticalCenter: parent.verticalCenter
                        width: 52; height: 40
                        radius: 8
                        color: ZTheme.surface
                        border.width: row.modelData.guess === "" ? 2 : 1
                        border.color: row.modelData.guess === "" ? ZTheme.accent : ZTheme.line
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        x: guessCell.visible ? (guessCell.width - width) / 2 : 0
                        text: root.discover ? (row.modelData.guess !== "" ? row.modelData.guess : "?")
                                            : row.modelData.expected
                        color: root.discover && row.modelData.guess === "" ? ZTheme.accentText : ZTheme.text
                        font.family: ZTheme.mono; font.pixelSize: 17; font.bold: root.discover
                    }
                }
                Text {
                    Layout.preferredWidth: root.colWidth
                    text: row.filled ? row.modelData.actual : (root.discover && !workbench.guessing ? "?" : "")
                    color: row.filled ? ZTheme.text : ZTheme.muted
                    font.family: ZTheme.mono; font.pixelSize: 17; font.bold: true
                }
                Text {
                    Layout.fillWidth: true
                    text: root.discover ? (row.modelData.match ?? "")
                                        : row.filled ? (row.modelData.ok ? "✓" : "✗") : ""
                    color: root.discover ? ZTheme.navy : row.modelData.ok ? ZTheme.ok : ZTheme.bad
                    font.pixelSize: 20; font.bold: true
                }
            }
        }
    }
}
