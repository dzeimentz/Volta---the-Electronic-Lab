// »?« in Geraetefenstern (Generator, Oszilloskop): blendet eine kurze Hilfe ein und aus (48 dp Tippflaeche),
// gerahmt wie die uebrigen Knoepfe der Geraete (PanelButton)
import QtQuick
import QtQuick.Controls.Basic

PanelButton {
    width: Theme.control
    height: Theme.control
    checkable: true
    text: "?"
    font.pixelSize: 17
    ToolTip.text: checked ? qsTr("Hilfe ausblenden") : qsTr("Kurze Hilfe")
    Accessible.name: ToolTip.text
}
