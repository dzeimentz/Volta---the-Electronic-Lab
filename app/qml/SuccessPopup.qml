// Glueckwunsch nach »Fertig« mit funktionierendem Aufbau (Lab.qml): Tick freut sich und sagt, wie es weitergeht
// (Beobachten bzw. Erklaerung). »Weiter«, Antippen daneben oder Escape schliesst.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: success
    property bool observe: true // danach kommt das Beobachten (sonst gleich die Erklaerung)

    objectName: "successPopup"
    modal: true
    focus: true
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape
    padding: 16
    background: Rectangle {
        radius: 12
        color: "#FFFDF7"
        border.color: "#2E7D32"
        border.width: 3
    }

    contentItem: Column {
        spacing: 12
        Label {
            width: parent.width
            text: "✓ " + qsTr("Richtig gelöst!")
            color: "#1F5E23"
            font.pixelSize: Math.round(22 * Theme.fontScale)
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
        TickSays {
            width: parent.width
            figure: 96
            mood: "froehlich"
            text: qsTr("Super, geschafft! Deine Schaltung funktioniert – das ist die richtige Lösung.") + " "
                  + (success.observe ? qsTr("Schau jetzt genau hin, was passiert.")
                                     : qsTr("Lies jetzt, warum es funktioniert."))
            bubbleColor: "#E6F4E7"
            borderColor: "#2E7D32"
        }
        Button {
            objectName: "successClose"
            anchors.right: parent.right
            height: Theme.control
            font.bold: true
            text: qsTr("Weiter")
            onClicked: success.close()
        }
    }
}
