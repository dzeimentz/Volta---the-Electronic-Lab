import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Werkzeugkiste: alle entdeckten Regeln (NICHT, UND, ODER) und eingepackten Erfindungen mit Merksatz;
// dazu die Wahl der Schaltzeichen (DIN oder US).
Popup {
    id: root
    modal: true
    focus: true
    padding: 24
    width: Math.min(parent ? parent.width - 80 : 800, 860)
    height: Math.min(parent ? parent.height - 80 : 600, 620)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? (parent.height - height) / 2 : 0
    background: Rectangle { radius: 20; color: ZTheme.surface; border.color: ZTheme.line }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14
        RowLayout {
            Layout.fillWidth: true
            Image {
                source: "qrc:/zuse/zetti/zetti_bauen.png"
                Layout.preferredWidth: 64; Layout.preferredHeight: 84
                fillMode: Image.PreserveAspectFit
                mipmap: true
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Deine Werkzeugkiste")
                color: ZTheme.navy
                font.pixelSize: 26; font.bold: true
            }
            ZButton { text: qsTr("Schließen"); onClicked: root.close() }
        }
        Text {
            visible: workbench.tools.length === 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Noch leer. Jede Regel, die du in einer Rätselkiste entdeckst, und jede Erfindung, die du "
                       + "einpackst, landet hier – und du kannst sie danach überall einbauen.")
            color: ZTheme.muted
            font.pixelSize: 16
        }
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 14
            rowSpacing: 14
            Repeater {
                model: workbench.tools
                delegate: Rectangle {
                    id: card
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 104
                    radius: 16
                    color: modelData.invention ? "#FFF7E6" : "#ECFAFD"
                    border.color: modelData.invention ? ZTheme.pad : ZTheme.accentDim
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 14
                        GateShape {
                            Layout.preferredWidth: 90; Layout.preferredHeight: 76
                            symbol: card.modelData.symbol
                            inverted: card.modelData.inverted
                            us: workbench.symbolStyle === "us"
                            stroke: ZTheme.navy
                            fontSize: card.modelData.symbol.length > 2 ? 26 : 30
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text {
                                text: (card.modelData.invention ? "★ " : "") + card.modelData.name
                                color: ZTheme.navy
                                font.pixelSize: 20; font.bold: true
                            }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: card.modelData.merksatz
                                color: ZTheme.text
                                font.pixelSize: 15
                            }
                        }
                    }
                }
            }
        }
        Item { Layout.fillHeight: true }
        RowLayout {
            spacing: 8
            Text {
                text: qsTr("Schaltzeichen:")
                color: ZTheme.muted
                font.pixelSize: 15
            }
            ZButton {
                text: "DIN"
                primary: workbench.symbolStyle === "din"
                onClicked: workbench.symbolStyle = "din"
            }
            ZButton {
                text: "US"
                primary: workbench.symbolStyle === "us"
                onClicked: workbench.symbolStyle = "us"
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("DIN EN 60617 (Europa) oder IEEE Std 91 (USA)")
                color: ZTheme.muted
                font.pixelSize: 13
            }
        }
    }
}
