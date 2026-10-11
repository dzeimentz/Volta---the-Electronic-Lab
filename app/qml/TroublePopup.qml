// Hinweis, wenn etwas kaputt geht oder nicht funktioniert (Lab.qml): Tick erschrickt bzw. fragt nach und bietet den
// Weg in die Fehlersuche an (ab Stufe 2 freigeschaltet, sonst in die Hilfe). Antippen daneben oder Escape schliesst.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: trouble
    property bool finderUnlocked: true
    property string mood: "ueberrascht"
    property string message: ""
    signal faultFinderRequested()
    signal helpRequested()

    function showDestroyed(name) {
        mood = "ueberrascht"
        message = qsTr("Oh! %1 ist kaputt gegangen. Willst du wissen, warum?").arg(name)
        open()
    }
    function showNotWorking() {
        mood = "fragend"
        message = qsTr("Tut sich nichts? Ich kann deinen Aufbau prüfen und dir zeigen, wo es hakt.")
        open()
    }

    objectName: "troublePopup"
    modal: false
    focus: true
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape
    padding: 14
    background: Rectangle {
        radius: 12
        color: "#FFFDF7"
        border.color: trouble.mood === "ueberrascht" ? Theme.signalOrange : Theme.accent
        border.width: 2
    }

    contentItem: Column {
        spacing: 10
        TickSays {
            width: parent.width
            figure: 84
            mood: trouble.mood
            text: trouble.message
            bubbleColor: trouble.mood === "ueberrascht" ? "#FFE9DC" : "#EEF5FF"
            borderColor: trouble.mood === "ueberrascht" ? Theme.signalOrange : Theme.accent
        }
        Row {
            anchors.right: parent.right
            spacing: 10
            Button {
                objectName: "troubleClose"
                height: Theme.control
                text: qsTr("Später")
                onClicked: trouble.close()
            }
            Button {
                objectName: "troubleFinder"
                height: Theme.control
                font.bold: true
                text: trouble.finderUnlocked ? qsTr("Zur Fehlersuche") : qsTr("In der Hilfe nachsehen")
                onClicked: {
                    trouble.close()
                    if (trouble.finderUnlocked) trouble.faultFinderRequested()
                    else trouble.helpRequested()
                }
            }
        }
    }
}
