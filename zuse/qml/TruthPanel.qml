import QtQuick
import QtQuick.Layouts

// Wahrheitstabelle der Stufe: Soll und (nach dem Pruefen) Ist je Zeile; Abweichungen mit ✗ und Rahmen,
// nicht nur mit Farbe.
ColumnLayout {
    id: root
    spacing: 4
    property var inputNames: []
    property var outputNames: []

    RowLayout {
        Layout.fillWidth: true
        Text {
            Layout.preferredWidth: 90
            text: root.inputNames.join(" ")
            color: ZTheme.muted; font.family: ZTheme.mono; font.pixelSize: 15
        }
        Text {
            Layout.preferredWidth: 70
            text: qsTr("soll")
            color: ZTheme.muted; font.family: ZTheme.mono; font.pixelSize: 15
        }
        Text {
            Layout.preferredWidth: 70
            text: qsTr("ist")
            color: ZTheme.muted; font.family: ZTheme.mono; font.pixelSize: 15
        }
    }
    Repeater {
        model: workbench.checkRows
        delegate: Rectangle {
            id: row
            required property var modelData
            readonly property bool checked: modelData.actual !== ""
            Layout.fillWidth: true
            implicitHeight: 34
            radius: 6
            color: checked ? (modelData.ok ? "#14321F" : "#3A1A1D") : ZTheme.surface
            border.width: checked && !modelData.ok ? 2 : 0
            border.color: ZTheme.bad
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                Text {
                    Layout.preferredWidth: 82
                    text: row.modelData.inputs.split("").join(" ")
                    color: ZTheme.text; font.family: ZTheme.mono; font.pixelSize: 17
                }
                Text {
                    Layout.preferredWidth: 70
                    text: row.modelData.expected
                    color: ZTheme.text; font.family: ZTheme.mono; font.pixelSize: 17
                }
                Text {
                    Layout.preferredWidth: 50
                    text: row.modelData.actual
                    color: ZTheme.text; font.family: ZTheme.mono; font.pixelSize: 17
                }
                Text {
                    Layout.fillWidth: true
                    text: row.checked ? (row.modelData.ok ? "✓" : "✗") : ""
                    color: row.modelData.ok ? ZTheme.ok : ZTheme.bad
                    font.pixelSize: 20; font.bold: true
                }
            }
        }
    }
}
