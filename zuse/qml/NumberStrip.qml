import QtQuick
import QtQuick.Layouts

// Zahlenleiste: Schalter und LEDs als Zahl – binaer mit Stellenwerten, dezimal und hexadezimal. Zuschaltbar
// (workbench.numbersVisible); »Warum?« oeffnet die Erklaerung.
Rectangle {
    id: root
    signal explain()
    radius: 16
    color: ZTheme.surface
    border.color: ZTheme.line

    component NumberBlock: RowLayout {
        id: block
        property string title
        property var number: ({})
        spacing: 6
        Text {
            text: block.title
            color: ZTheme.navy
            font.pixelSize: 15; font.bold: true
            Layout.alignment: Qt.AlignVCenter
        }
        Repeater { // Bits mit Stellenwert darunter (hoechstes Bit links)
            model: block.number.bits || 0
            delegate: Column {
                required property int index
                readonly property string bit: (block.number.bin || "").charAt(index)
                spacing: 2
                Rectangle {
                    width: 30; height: 34; radius: 6
                    color: bit === "1" ? ZTheme.accent : ZTheme.surfaceHigh
                    border.color: bit === "1" ? ZTheme.accentText : ZTheme.line
                    Text {
                        anchors.centerIn: parent
                        text: bit
                        color: bit === "1" ? ZTheme.textOnAccent : ZTheme.text
                        font.family: ZTheme.mono; font.pixelSize: 20; font.bold: true
                    }
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: String(Math.pow(2, (block.number.bits || 0) - 1 - index))
                    color: ZTheme.muted
                    font.pixelSize: 12
                }
            }
        }
        Text { text: "="; color: ZTheme.muted; font.pixelSize: 20; Layout.alignment: Qt.AlignVCenter }
        Column {
            Layout.alignment: Qt.AlignVCenter
            Text { text: (block.number.dec || "?"); color: ZTheme.text; font.family: ZTheme.mono; font.pixelSize: 20; font.bold: true }
            Text { text: qsTr("dez"); color: ZTheme.muted; font.pixelSize: 12 }
        }
        Column {
            Layout.alignment: Qt.AlignVCenter
            Text { text: "0x" + (block.number.hex || "?"); color: ZTheme.text; font.family: ZTheme.mono; font.pixelSize: 20; font.bold: true }
            Text { text: qsTr("hex"); color: ZTheme.muted; font.pixelSize: 12 }
        }
    }

    // Zahlen als Fluss: reicht die Breite nicht (z. B. A, B und fuenf LEDs), rutschen sie in eine zweite Zeile
    implicitHeight: Math.max(92, numbers.implicitHeight + 24)
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 10
        spacing: 10
        Flow {
            id: numbers
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 18
            Repeater { // Schalter als eine Zahl oder in Gruppen (z. B. A und B)
                model: workbench.inputNumbers
                delegate: NumberBlock {
                    required property var modelData
                    title: modelData.title !== "" ? modelData.title : qsTr("Schalter")
                    number: modelData
                }
            }
            NumberBlock { title: qsTr("LEDs"); number: workbench.outputNumber }
        }
        ZButton { text: qsTr("Warum?"); onClicked: root.explain() }
        ZButton {
            text: "×"
            font.pixelSize: 22
            onClicked: workbench.numbersVisible = false
            Accessible.name: qsTr("Zahlenleiste ausblenden")
        }
    }
}
