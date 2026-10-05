// Kurzer Hinweis (z. B. warum ein Bauteil nicht passt); verschwindet nach wenigen Sekunden.
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: toast
    property alias text: label.text

    function show(message) {
        label.text = message
        opacity = 1
        hideTimer.restart()
    }

    radius: 8
    color: Theme.navy
    opacity: 0
    visible: opacity > 0
    width: label.implicitWidth + 32
    height: label.implicitHeight + 20
    Behavior on opacity { NumberAnimation { duration: 200 } }

    Label {
        id: label
        anchors.centerIn: parent
        color: "white"
        font.pixelSize: 15
    }

    Timer {
        id: hideTimer
        interval: 3500
        onTriggered: toast.opacity = 0
    }
}
