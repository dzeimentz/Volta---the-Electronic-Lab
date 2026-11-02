// »?« in Geraetefenstern (Generator, Oszilloskop): blendet eine kurze Hilfe ein und aus (48 dp Tippflaeche)
import QtQuick
import QtQuick.Controls.Basic

ToolButton {
    id: toggle
    width: Theme.control
    height: Theme.control
    checkable: true
    ToolTip.visible: hovered
    ToolTip.text: checked ? qsTr("Hilfe ausblenden") : qsTr("Kurze Hilfe")
    Accessible.name: ToolTip.text
    background: Item {}
    contentItem: Item {
        Rectangle {
            anchors.centerIn: parent
            width: 26; height: 26; radius: 13
            color: toggle.checked ? Theme.navy : (toggle.hovered ? "#E6EEF6" : "#F2F5F8")
            border.color: Theme.navy
            border.width: 1.5
            Text {
                anchors.centerIn: parent
                text: "?"
                color: toggle.checked ? "white" : Theme.navy
                font.pixelSize: 15
                font.bold: true
            }
        }
    }
}
