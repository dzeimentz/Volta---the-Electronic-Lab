// Zeichen der Hilfe: rotes Fragezeichen auf gelbem Kreis mit dunklem Rand (gut erkennbar, auch auf dunklem Grund).
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: icon
    property real size: 28
    width: size
    height: size
    radius: size / 2
    color: "#FFD23F"
    border.color: "#1E2B38"
    border.width: Math.max(1.5, size / 14)
    Label {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 1
        text: "?"
        color: "#C62828"
        font.pixelSize: Math.round(icon.size * 0.72)
        font.bold: true
    }
    Accessible.ignored: true
}
