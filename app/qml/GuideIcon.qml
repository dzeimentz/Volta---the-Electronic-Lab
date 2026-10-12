// Symbol der Kurzanleitung: Blatt mit gelbem Kopf und drei nummerierten Zeilen (neben dem Hilfe-Fragezeichen)
import QtQuick

Item {
    id: icon
    property real size: 28
    width: size; height: size

    Rectangle {
        anchors.centerIn: parent
        width: icon.size * 0.78
        height: icon.size
        radius: icon.size * 0.1
        color: "white"
        border.color: Theme.navy
        border.width: Math.max(1.5, icon.size * 0.07)
        clip: true
        Rectangle { width: parent.width; height: parent.height * 0.26; color: "#FFD23F" }
        Rectangle {
            width: parent.width; height: parent.border.width
            y: parent.height * 0.26
            color: Theme.navy
        }
        Column {
            x: parent.width * 0.16
            y: parent.height * 0.40
            spacing: icon.size * 0.09
            Repeater {
                model: 3
                Row {
                    spacing: icon.size * 0.06
                    Rectangle { width: icon.size * 0.09; height: width; radius: width / 2; color: "#D32F2F" }
                    Rectangle { width: icon.size * 0.36; height: icon.size * 0.07; radius: height / 2; color: Theme.navy
                                anchors.verticalCenter: parent.verticalCenter }
                }
            }
        }
    }
}
