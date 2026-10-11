// Aufgabe fuer die Klasse anzeigen (Lehrkraft, aus dem Elternbereich): grosser QR-Code zum Scannen mit der
// Kamera-App, daneben der Code zum Eintippen (falls die Aufgabe hineinpasst) und was die Aufgabe enthaelt.
// Fuer Beamer und Tafel: fuellt das ganze Fenster.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: dlg
    required property var controller
    property string link: ""
    property string code: ""
    property var info: ({})

    function showTask(newLink, newCode) {
        link = newLink
        code = newCode
        info = controller.readTask(newLink)
        open()
    }

    objectName: "taskQrDialog"
    anchors.centerIn: Overlay.overlay
    width: Overlay.overlay ? Overlay.overlay.width : 800
    height: Overlay.overlay ? Overlay.overlay.height : 600
    modal: true
    focus: true
    padding: 24
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { color: "white" }

    contentItem: Item {
        Row {
            anchors.fill: parent
            spacing: 32
            QrCanvas {
                id: qr
                objectName: "taskQrCanvas"
                width: Math.min(parent.height, parent.width * 0.58)
                height: width
                anchors.verticalCenter: parent.verticalCenter
                code: dlg.opened && dlg.link !== "" ? dlg.controller.qrCode(dlg.link) : ({})
            }
            Column {
                width: parent.width - qr.width - 32
                anchors.verticalCenter: parent.verticalCenter
                spacing: 16
                Row {
                    spacing: 12
                    Tick { visible: Theme.tick; mood: "erklaerend"; width: 84; height: 84 }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Aufgabe für die Klasse")
                        font.pixelSize: 28
                        font.bold: true
                        color: Theme.navy
                    }
                }
                Label {
                    width: parent.width
                    wrapMode: Text.Wrap
                    text: qsTr("Scannt den Code mit der Kamera-App. Volta öffnet sich und zeigt die Aufgabe.")
                    font.pixelSize: 20
                    color: "#1E2B38"
                }
                Column {
                    visible: dlg.code !== ""
                    spacing: 4
                    Label {
                        text: qsTr("Oder in Volta unter »Aufgabe« eintippen:")
                        font.pixelSize: 18
                        color: "#4A5866"
                    }
                    Label {
                        objectName: "taskCodeText"
                        text: dlg.code
                        font.pixelSize: 48
                        font.bold: true
                        font.family: "monospace"
                        font.letterSpacing: 4
                        color: Theme.navy
                    }
                }
                Label {
                    visible: (dlg.info.text || "") !== ""
                    width: parent.width
                    wrapMode: Text.Wrap
                    text: "»" + (dlg.info.text || "") + "«"
                    font.pixelSize: 20
                    font.italic: true
                    color: "#1E2B38"
                }
                Repeater {
                    model: dlg.info.experiments || []
                    Label {
                        required property var modelData
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: "•  " + qsTr("Stufe %1: %2").arg(modelData.stage).arg(modelData.title)
                        font.pixelSize: 18
                        color: "#1E2B38"
                    }
                }
                Label {
                    visible: dlg.info.hasBoard === true
                    width: parent.width
                    wrapMode: Text.Wrap
                    text: "•  " + qsTr("Eigener Aufbau: %1").arg(dlg.info.title || qsTr("ohne Titel"))
                    font.pixelSize: 18
                    color: "#1E2B38"
                }
                Button {
                    objectName: "taskQrClose"
                    height: Theme.control
                    text: qsTr("Schließen")
                    font.bold: true
                    onClicked: dlg.close()
                }
            }
        }
    }
}
