// Kleines Kontaktfeld im Text (Aufbauschritte): sieht aus wie das Feld auf der Grundplatte bzw. am Pult,
// mit Namen und fuenf Steckplaetzen. Grundplatte: Hintergrund in der Farbe der Reihe (Theme.fieldTint).
import QtQuick

Rectangle {
    id: chip
    property string name
    property int columns: 5 // Spaltenzahl der Grundplatte (Verlauf der Reihenfarbe)

    readonly property int row: "ABCDE".indexOf(name.charAt(0))
    readonly property bool boardField: /^[A-E](1[0-2]|[1-9])$/.test(name)
    readonly property bool plus: name === "V+" || name === "+B2"
    readonly property bool minus2: name === "\u2212B2" // Minuspol von Batterie 2: blau
    readonly property bool minus: name === "0V"
    readonly property bool moduleField: /^M\d+$/.test(name)

    width: 46
    height: 34
    radius: 6
    color: boardField ? Theme.fieldTint(row, parseInt(name.substring(1)), Math.max(columns, parseInt(name.substring(1))))
         : moduleField ? "#D8CCE8" : Theme.desk
    Accessible.name: name

    Rectangle {
        anchors.fill: parent
        anchors.margins: 3
        radius: 4
        color: chip.plus ? "#C62828" : chip.minus ? "#17191C" : chip.minus2 ? "#2C62C9" : "transparent"
        gradient: chip.plus || chip.minus || chip.minus2 ? null : metal
        border.color: Theme.fieldEdge
        Gradient {
            id: metal
            GradientStop { position: 0.0; color: Theme.fieldShine }
            GradientStop { position: 0.42; color: Theme.fieldMetal }
            GradientStop { position: 0.58; color: Theme.fieldShade }
            GradientStop { position: 1.0; color: Theme.fieldGlow }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 1
            text: chip.name
            color: chip.plus || chip.minus ? "white" : Theme.navy
            font.pixelSize: 12
            font.bold: true
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
            spacing: 2
            Repeater {
                model: 5
                Rectangle { width: 4; height: 4; radius: 1; color: chip.plus || chip.minus ? "#9AA3AD" : Theme.socket }
            }
        }
    }
}
