import QtQuick
import QtQuick.Layouts

// Tabelle der Stufe. Entdecken: was die eingesetzte Kiste bei jeder ausprobierten Schalterstellung gemacht hat
// (aktuelle Stellung umrandet). Bauen: Soll und nach dem Pruefen Ist; Abweichungen mit ✗ und Rahmen, nicht nur Farbe.
ColumnLayout {
    id: root
    spacing: 6
    readonly property bool discover: workbench.stageKind === "entdecken"
    readonly property var inputs: workbench.panelInputs
    readonly property var outputs: workbench.panelOutputs

    // Legende: Buchstabe = Bedeutung
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
    Item { Layout.preferredHeight: 4 }
    RowLayout {
        Layout.fillWidth: true
        Text {
            Layout.preferredWidth: 92
            text: root.inputs.map(p => p.name).join(" ")
            color: ZTheme.muted; font.family: ZTheme.mono; font.pixelSize: 15; font.bold: true
        }
        Text {
            visible: !root.discover
            Layout.preferredWidth: 64
            text: qsTr("soll")
            color: ZTheme.muted; font.pixelSize: 15; font.bold: true
        }
        Text {
            Layout.preferredWidth: 64
            text: root.discover ? root.outputs.map(p => p.name).join(" ") : qsTr("ist")
            color: ZTheme.muted; font.pixelSize: 15; font.bold: true
        }
    }
    Repeater {
        model: workbench.checkRows
        delegate: Rectangle {
            id: row
            required property var modelData
            readonly property bool filled: modelData.actual !== ""
            readonly property bool wrong: !root.discover && filled && !modelData.ok
            Layout.fillWidth: true
            implicitHeight: 36
            radius: 8
            color: root.discover ? (filled ? "#ECFAFD" : ZTheme.surfaceHigh)
                                 : (filled ? (modelData.ok ? ZTheme.okBg : ZTheme.badBg) : ZTheme.surfaceHigh)
            border.width: wrong || (root.discover && !!modelData.current) ? 2 : 0
            border.color: wrong ? ZTheme.bad : ZTheme.accent
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                Text {
                    Layout.preferredWidth: 84
                    text: row.modelData.inputs.split("").join(" ")
                    color: ZTheme.text; font.family: ZTheme.mono; font.pixelSize: 17; font.bold: true
                }
                Text {
                    visible: !root.discover
                    Layout.preferredWidth: 64
                    text: row.modelData.expected
                    color: ZTheme.text; font.family: ZTheme.mono; font.pixelSize: 17
                }
                Text {
                    Layout.preferredWidth: 64
                    text: row.filled ? row.modelData.actual : (root.discover ? "?" : "")
                    color: row.filled ? ZTheme.text : ZTheme.muted
                    font.family: ZTheme.mono; font.pixelSize: 17; font.bold: true
                }
                Text {
                    Layout.fillWidth: true
                    visible: !root.discover
                    text: row.filled ? (row.modelData.ok ? "✓" : "✗") : ""
                    color: row.modelData.ok ? ZTheme.ok : ZTheme.bad
                    font.pixelSize: 20; font.bold: true
                }
            }
        }
    }
}
