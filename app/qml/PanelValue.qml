// Eingestellter Wert in Geraetefenstern (Zeit/Div, V/Div, Triggerpegel, Pruefspannung, Strombegrenzung): gerahmt wie
// die Knoepfe (PanelButton), aber weiss mit fetter Schrift, damit er sich als Anzeige von den Knoepfen abhebt
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    property alias text: valueText.text
    height: Theme.control
    radius: 6
    color: "white"
    border.color: "#7F93A8"
    border.width: 1.5
    Label {
        id: valueText
        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: Theme.navy
        font.pixelSize: Math.round(14 * Theme.fontScale)
        font.bold: true
    }
}
