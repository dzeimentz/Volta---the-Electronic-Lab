import QtQuick

// Loetpad eines Anschlusses (Bedienziel 56 dp). Pegel: 1 Akzent gefuellt, 0 grau hohl, ? mit Fragezeichen.
Item {
    id: root
    property string endpoint
    property string level: "?"
    property bool highlight: false
    width: ZTheme.pin
    height: ZTheme.pin

    Rectangle {
        anchors.centerIn: parent
        width: 22; height: 22; radius: 11
        color: root.level === "1" ? ZTheme.accent : ZTheme.board
        border.width: root.highlight ? 4 : 3
        border.color: root.highlight ? ZTheme.accent : ZTheme.pad
        Text {
            anchors.centerIn: parent
            visible: root.level === "?"
            text: "?"
            color: ZTheme.muted
            font.family: ZTheme.mono
            font.pixelSize: 13
            font.bold: true
        }
    }
}
