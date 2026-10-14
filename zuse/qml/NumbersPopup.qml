import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Zetti erklaert die Zahlensysteme: warum ein Computer binaer rechnet, wozu Hexadezimal; Vergleich 0 bis 15
// mit der aktuellen Schalterstellung hervorgehoben.
Popup {
    id: root
    modal: true
    focus: true
    padding: 24
    width: Math.min(parent ? parent.width - 60 : 1000, 1060)
    height: Math.min(parent ? parent.height - 60 : 700, 720)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? (parent.height - height) / 2 : 0
    background: Rectangle { radius: 20; color: ZTheme.surface; border.color: ZTheme.line }

    readonly property int current: workbench.inputNumber.value

    component Explain: Rectangle {
        id: card
        property string title
        property string base
        property string body
        property color tint
        Layout.fillWidth: true
        Layout.preferredHeight: col.implicitHeight + 24
        radius: 14
        color: tint
        border.color: ZTheme.line
        ColumnLayout {
            id: col
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
            spacing: 4
            RowLayout {
                Text { text: card.title; color: ZTheme.navy; font.pixelSize: 19; font.bold: true }
                Text { text: card.base; color: ZTheme.muted; font.pixelSize: 14 }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: card.body
                color: ZTheme.text
                font.pixelSize: 15
                lineHeight: 1.1
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 20
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12
            RowLayout {
                Layout.fillWidth: true
                Image {
                    source: "qrc:/zuse/zetti/zetti_erklaerend.png"
                    Layout.preferredWidth: 70; Layout.preferredHeight: 94
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                }
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: qsTr("Drei Arten, dieselbe Zahl zu schreiben")
                    color: ZTheme.navy
                    font.pixelSize: 24; font.bold: true
                }
            }
            Explain {
                title: qsTr("Dezimal")
                base: qsTr("Basis 10 · Ziffern 0–9")
                tint: "#F4F6F8"
                body: qsTr("So zählen wir Menschen – mit zehn Ziffern, vermutlich weil wir zehn Finger haben. "
                           + "Jede Stelle ist zehnmal so viel wert wie die rechts daneben: 1, 10, 100 …")
            }
            Explain {
                title: qsTr("Binär")
                base: qsTr("Basis 2 · Ziffern 0 und 1")
                tint: "#ECFAFD"
                body: qsTr("Ein Computer kennt nur zwei Zustände: Strom an oder aus. Zwei Zustände lassen sich "
                           + "mit Schaltern und Transistoren sehr sicher auseinanderhalten – auch wenn die Spannung "
                           + "etwas schwankt. Mit zehn Spannungsstufen gäbe es dauernd Verwechslungen. Darum rechnet "
                           + "ein Computer binär. Jede Stelle ist doppelt so viel wert wie die rechts daneben: "
                           + "8, 4, 2, 1. So ist 0101 = 4 + 1 = 5.")
            }
            Explain {
                title: qsTr("Hexadezimal")
                base: qsTr("Basis 16 · Ziffern 0–9 und A–F")
                tint: "#FFF7E6"
                body: qsTr("Lange Binärzahlen sind für Menschen schwer zu lesen: 1101 0110. Je vier Bits passen "
                           + "genau in eine Hex-Ziffer (0–9, dann A = 10 bis F = 15). Aus 1101 0110 wird D6 – kurz "
                           + "und ohne Rechnen umzuwandeln. Deshalb schreiben Programmierer Speicheradressen, "
                           + "Maschinenbefehle und Farben (#FF0000 = Rot) in Hex. Der Computer selbst rechnet weiter "
                           + "binär; Hex ist nur eine Schreibweise für uns.")
            }
            Item { Layout.fillHeight: true }
            RowLayout {
                ZButton {
                    text: workbench.numbersVisible ? qsTr("Zahlenleiste ausblenden") : qsTr("Zahlenleiste anzeigen")
                    onClicked: workbench.numbersVisible = !workbench.numbersVisible
                }
                Item { Layout.fillWidth: true }
                ZButton { text: qsTr("Schließen"); primary: true; onClicked: root.close() }
            }
        }
        ColumnLayout { // Vergleich 0 bis 15
            Layout.preferredWidth: 270
            Layout.fillWidth: false
            Layout.fillHeight: true
            spacing: 2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.preferredWidth: 70; text: qsTr("dezimal"); color: ZTheme.muted; font.pixelSize: 13; font.bold: true }
                Text { Layout.preferredWidth: 110; text: qsTr("binär"); color: ZTheme.muted; font.pixelSize: 13; font.bold: true }
                Text { Layout.fillWidth: true; text: qsTr("hex"); color: ZTheme.muted; font.pixelSize: 13; font.bold: true }
            }
            Repeater {
                model: 16
                delegate: Rectangle {
                    id: row
                    required property int index
                    readonly property bool now: index === root.current
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    radius: 6
                    color: now ? ZTheme.accent : (index % 2 ? ZTheme.surface : ZTheme.surfaceHigh)
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        Text {
                            Layout.preferredWidth: 60
                            text: String(row.index)
                            color: row.now ? ZTheme.textOnAccent : ZTheme.text
                            font.family: ZTheme.mono; font.pixelSize: 17; font.bold: true
                        }
                        Text {
                            Layout.preferredWidth: 110
                            text: ("000" + row.index.toString(2)).slice(-4)
                            color: row.now ? ZTheme.textOnAccent : ZTheme.text
                            font.family: ZTheme.mono; font.pixelSize: 17
                        }
                        Text {
                            Layout.fillWidth: true
                            text: row.index.toString(16).toUpperCase()
                            color: row.now ? ZTheme.textOnAccent : ZTheme.text
                            font.family: ZTheme.mono; font.pixelSize: 17; font.bold: true
                        }
                    }
                }
            }
        }
    }
}
